#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/util/str.h"
#include "math-viewer/math_viewer.h"

struct _MathViewer
{
    GtkBox parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_tf_expr* tf;
    const struct csfg_expr_pool* pool;
    int expr;
};

G_DEFINE_DYNAMIC_TYPE(MathViewer, math_viewer, GTK_TYPE_BOX)

enum csfg_layout_type
{
    CSFG_LAYOUT_LIT,
    CSFG_LAYOUT_VAR,
    CSFG_LAYOUT_INF,
    CSFG_LAYOUT_NEG,
    CSFG_LAYOUT_ADD,
    CSFG_LAYOUT_SUB,
    CSFG_LAYOUT_MUL,
    CSFG_LAYOUT_FRAC,
    CSFG_LAYOUT_PARENS
};

struct csfg_layout_node
{
    union
    {
        struct
        {
            float lit;
        };

        struct
        {
            int idx;
        } var;

        struct
        {
            int op_adv_x;
        } binop;

        struct
        {
            int adv_x;
        } paren;

        struct
        {
            int dist_to_frac_line;
            int num_x;
            int den_x;
        } frac;
    };

    int adv_x, height;
    int child[2];
    enum csfg_layout_type type;
};

VEC_DECLARE(csfg_layout_vec, struct csfg_layout_node, 16)
VEC_DEFINE(csfg_layout_vec, struct csfg_layout_node, 16)

struct csfg_layout
{
    struct csfg_layout_vec* nodes;
    struct str* str;
};

static const char utf8_infinity[] = {0xE2, 0x88, 0x9E, 0x00};
static const char utf8_cdot[]     = {0xC2, 0xB7, 0x00};

/* -------------------------------------------------------------------------- */
static int imax(int a, int b)
{
    return a > b ? a : b;
}
static int new_node(
    struct csfg_layout_vec** nodes,
    enum csfg_layout_type type,
    int adv_x,
    int height)
{
    struct csfg_layout_node* node = csfg_layout_vec_emplace(nodes);
    if (node == NULL)
        return -1;

    node->type     = type;
    node->adv_x    = adv_x;
    node->height   = height;
    node->child[0] = -1;
    node->child[1] = -1;

    return vec_count(*nodes) - 1;
}
static int
new_node_var(struct csfg_layout_vec** nodes, int adv_x, int height, int var_idx)
{
    int n = new_node(nodes, CSFG_LAYOUT_VAR, adv_x, height);
    if (n < 0)
        return -1;
    vec_get(*nodes, n)->var.idx = var_idx;
    return n;
}
static int new_node_binop(
    struct csfg_layout_vec** nodes,
    enum csfg_layout_type type,
    int op_adv_x,
    int op_height,
    int left,
    int right)
{
    int n, combined_height, combined_adv_x;
    if (left < 0 || right < 0)
        return -1;

    combined_height =
        imax(vec_get(*nodes, left)->height, vec_get(*nodes, right)->height);
    combined_adv_x =
        vec_get(*nodes, left)->adv_x + vec_get(*nodes, right)->adv_x;

    n = new_node(
        nodes,
        type,
        op_adv_x + combined_adv_x,
        imax(op_height, combined_height));
    if (n < 0)
        return -1;

    vec_get(*nodes, n)->binop.op_adv_x = op_adv_x;
    vec_get(*nodes, n)->child[0]       = left;
    vec_get(*nodes, n)->child[1]       = right;

    return n;
}
static int new_node_add(
    struct csfg_layout_vec** nodes,
    int op_adv_x,
    int op_height,
    int left,
    int right)
{
    return new_node_binop(
        nodes, CSFG_LAYOUT_ADD, op_adv_x, op_height, left, right);
}
static int new_node_mul(
    struct csfg_layout_vec** nodes,
    int op_adv_x,
    int op_height,
    int left,
    int right)
{
    return new_node_binop(
        nodes, CSFG_LAYOUT_MUL, op_adv_x, op_height, left, right);
}
static int
new_node_parens(struct csfg_layout_vec** nodes, int paren_adv_x, int child)
{
    int n;
    if (child < 0)
        return -1;
    n = new_node(
        nodes,
        CSFG_LAYOUT_PARENS,
        paren_adv_x * 2 + vec_get(*nodes, child)->adv_x,
        vec_get(*nodes, child)->height);
    if (n < 0)
        return -1;

    vec_get(*nodes, n)->paren.adv_x = paren_adv_x;
    vec_get(*nodes, n)->child[0]    = child;

    return n;
}
static int new_node_frac(
    struct csfg_layout_vec** nodes, int num, int den, int dist_to_frac_line)
{
    int n, max_width, total_height;
    if (num < 0 || den < 0)
        return -1;

    max_width = imax(vec_get(*nodes, num)->adv_x, vec_get(*nodes, den)->adv_x);
    total_height = vec_get(*nodes, num)->height + vec_get(*nodes, den)->height;
    total_height += dist_to_frac_line;

    n = new_node(nodes, CSFG_LAYOUT_FRAC, max_width, total_height);
    if (n < 0)
        return -1;

    vec_get(*nodes, n)->child[0]               = num;
    vec_get(*nodes, n)->child[1]               = den;
    vec_get(*nodes, n)->frac.dist_to_frac_line = dist_to_frac_line;
    vec_get(*nodes, n)->frac.num_x =
        (max_width - vec_get(*nodes, num)->adv_x) / 2;
    vec_get(*nodes, n)->frac.den_x =
        (max_width - vec_get(*nodes, den)->adv_x) / 2;

    return n;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_layout(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int expr,
    cairo_t* cr);

static int layout_lit(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int expr,
    cairo_t* cr)
{
    cairo_text_extents_t ext;
    double value = pool->nodes[expr].value.lit;
    str_clear(layout->str);
    if (str_append_float(&layout->str, value) != 0)
        return -1;
    cairo_text_extents(cr, str_cstr(layout->str), &ext);
    return new_node(&layout->nodes, CSFG_LAYOUT_LIT, ext.x_advance, ext.height);
}

static int layout_var(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int expr,
    cairo_t* cr)
{
    cairo_text_extents_t ext;
    int var_idx      = pool->nodes[expr].value.var_idx;
    const char* cstr = strlist_cstr(pool->var_names, var_idx);
    cairo_text_extents(cr, cstr, &ext);
    return new_node_var(&layout->nodes, ext.x_advance, ext.height, var_idx);
}

static int layout_inf(struct csfg_layout* layout, cairo_t* cr)
{
    cairo_text_extents_t ext;
    cairo_text_extents(cr, utf8_infinity, &ext);
    return new_node(&layout->nodes, CSFG_LAYOUT_INF, ext.x_advance, ext.height);
}

static int layout_neg(struct csfg_layout* layout, cairo_t* cr)
{
    cairo_text_extents_t ext;
    cairo_text_extents(cr, "-", &ext);
    return new_node(&layout->nodes, CSFG_LAYOUT_NEG, ext.width, ext.height);
}

static int layout_sub(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int left,
    int right,
    cairo_t* cr)
{
    cairo_text_extents_t ext;
    int need_parens = (pool->nodes[right].type == CSFG_EXPR_ADD);
    left            = csfg_expr_layout(layout, pool, left, cr);
    right           = csfg_expr_layout(layout, pool, right, cr);

    if (need_parens)
    {
        cairo_text_extents(cr, "(", &ext);
        right = new_node_parens(&layout->nodes, ext.x_advance, right);
    }

    cairo_text_extents(cr, "-", &ext);
    return new_node_binop(
        &layout->nodes,
        CSFG_LAYOUT_SUB,
        ext.x_advance,
        ext.height,
        left,
        right);
}

static int layout_add(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int left,
    int right,
    cairo_t* cr)
{
    cairo_text_extents_t ext;
    if (pool->nodes[right].type == CSFG_EXPR_NEG)
        return layout_sub(layout, pool, left, pool->nodes[right].child[0], cr);

    left  = csfg_expr_layout(layout, pool, left, cr);
    right = csfg_expr_layout(layout, pool, right, cr);

    cairo_text_extents(cr, "+", &ext);
    return new_node_binop(
        &layout->nodes,
        CSFG_LAYOUT_ADD,
        ext.x_advance,
        ext.height,
        left,
        right);
}

static int layout_frac(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int left,
    int right,
    double den_exp,
    cairo_t* cr)
{
    left  = csfg_expr_layout(layout, pool, left, cr);
    right = csfg_expr_layout(layout, pool, right, cr);

    return new_node_frac(&layout->nodes, left, right, 5);
}

static int layout_mul(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int left,
    int right,
    cairo_t* cr)
{
    cairo_text_extents_t ext;
    int need_lparens, need_rparens;

    if (pool->nodes[right].type == CSFG_EXPR_POW)
    {
        int exp = pool->nodes[right].child[1];
        if (pool->nodes[exp].type == CSFG_EXPR_LIT &&
            pool->nodes[exp].value.lit < 0.0)
        {
            int base         = pool->nodes[right].child[0];
            double exp_value = pool->nodes[exp].value.lit;
            return layout_frac(layout, pool, left, base, exp_value, cr);
        }
    }

    need_lparens = (pool->nodes[left].type == CSFG_EXPR_ADD);
    need_rparens = (pool->nodes[right].type == CSFG_EXPR_ADD);

    left  = csfg_expr_layout(layout, pool, left, cr);
    right = csfg_expr_layout(layout, pool, right, cr);

    if (need_lparens)
    {
        cairo_text_extents(cr, "(", &ext);
        left = new_node_parens(&layout->nodes, ext.x_advance, left);
    }
    if (need_rparens)
    {
        cairo_text_extents(cr, "(", &ext);
        right = new_node_parens(&layout->nodes, ext.x_advance, right);
    }

    cairo_text_extents(cr, utf8_cdot, &ext);
    return new_node_mul(&layout->nodes, ext.x_advance, ext.height, left, right);
}

/* -------------------------------------------------------------------------- */
int csfg_expr_layout(
    struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int expr,
    cairo_t* cr)
{
    int left, right, need_lparens, need_rparens;
    cairo_text_extents_t ext;

    enum csfg_expr_type type = pool->nodes[expr].type;
    switch (type)
    {
        case CSFG_EXPR_GC : CSFG_DEBUG_ASSERT(0); return -1;
        case CSFG_EXPR_LIT: return layout_lit(layout, pool, expr, cr);
        case CSFG_EXPR_VAR: return layout_var(layout, pool, expr, cr);
        case CSFG_EXPR_INF: return layout_inf(layout, cr);
        case CSFG_EXPR_NEG: return layout_neg(layout, cr);
        case CSFG_EXPR_ADD:
            return layout_add(
                layout,
                pool,
                pool->nodes[expr].child[0],
                pool->nodes[expr].child[1],
                cr);
        case CSFG_EXPR_MUL:
            return layout_mul(
                layout,
                pool,
                pool->nodes[expr].child[0],
                pool->nodes[expr].child[1],
                cr);
        case CSFG_EXPR_POW:
            left  = pool->nodes[expr].child[0];
            right = pool->nodes[expr].child[1];

            if (pool->nodes[left].type == CSFG_EXPR_MUL ||
                pool->nodes[left].type == CSFG_EXPR_ADD)
                need_lparens = 1;
            if (pool->nodes[right].type == CSFG_EXPR_MUL ||
                pool->nodes[right].type == CSFG_EXPR_ADD)
                need_rparens = 1;

            left  = csfg_expr_layout(layout, pool, left, cr);
            right = csfg_expr_layout(layout, pool, right, cr);
            if (left < 0 || right < 0)
                return -1;
            if (need_lparens)
            {
                cairo_text_extents(cr, "(", &ext);
                left = new_node_parens(&layout->nodes, ext.x_advance, left);
            }
            if (need_rparens)
            {
                cairo_text_extents(cr, "(", &ext);
                right = new_node_parens(&layout->nodes, ext.x_advance, right);
            }

            return -1;
            return new_node_frac(&layout->nodes, left, right, 5);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_draw(
    const struct csfg_layout* layout,
    const struct csfg_expr_pool* pool,
    int n,
    int x,
    int y,
    cairo_t* cr)
{
    enum csfg_layout_type type;
    int var_idx, left, right, num, den;
    const char* cstr;

    type = vec_get(layout->nodes, n)->type;
    switch (type)
    {
        case CSFG_LAYOUT_LIT: return -1;
        case CSFG_LAYOUT_VAR:
            var_idx = vec_get(layout->nodes, n)->var.idx;
            cstr    = strlist_cstr(pool->var_names, var_idx);
            cairo_move_to(cr, x, y);
            cairo_show_text(cr, cstr);
            break;

        case CSFG_LAYOUT_INF: return -1;
        case CSFG_LAYOUT_NEG: return -1;
        case CSFG_LAYOUT_ADD:
        case CSFG_LAYOUT_SUB:
        case CSFG_LAYOUT_MUL:
            left  = vec_get(layout->nodes, n)->child[0];
            right = vec_get(layout->nodes, n)->child[1];

            csfg_expr_draw(layout, pool, left, x, y, cr);
            x += vec_get(layout->nodes, left)->adv_x;

            cairo_move_to(cr, x, y);
            cairo_show_text(
                cr,
                type == CSFG_LAYOUT_ADD   ? "+"
                : type == CSFG_LAYOUT_SUB ? "-"
                                          : utf8_cdot);
            x += vec_get(layout->nodes, n)->binop.op_adv_x;

            csfg_expr_draw(layout, pool, right, x, y, cr);
            break;

        case CSFG_LAYOUT_FRAC:
            num = vec_get(layout->nodes, n)->child[0];
            den = vec_get(layout->nodes, n)->child[1];

            csfg_expr_draw(
                layout,
                pool,
                num,
                x + vec_get(layout->nodes, n)->frac.num_x,
                y,
                cr);
            y += vec_get(layout->nodes, n)->frac.dist_to_frac_line;

            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x + vec_get(layout->nodes, n)->adv_x, y);
            cairo_stroke(cr);
            y += vec_get(layout->nodes, num)->height;
            y += vec_get(layout->nodes, n)->frac.dist_to_frac_line;

            csfg_expr_draw(
                layout,
                pool,
                den,
                x + vec_get(layout->nodes, n)->frac.den_x,
                y,
                cr);

            break;

        case CSFG_LAYOUT_PARENS:
            left = vec_get(layout->nodes, n)->child[0];

            cairo_move_to(cr, x, y);
            cairo_show_text(cr, "(");
            x += vec_get(layout->nodes, n)->paren.adv_x;

            csfg_expr_draw(layout, pool, left, x, y, cr);
            x += vec_get(layout->nodes, left)->adv_x;

            cairo_move_to(cr, x, y);
            cairo_show_text(cr, ")");
            break;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void draw_tf(
    cairo_t* cr,
    const struct csfg_expr_pool* pool,
    const struct csfg_tf_expr* tf)
{
    struct str* str;
    PangoLayout* layout;
    PangoFontDescription* desc;
    int tw, th;
    int widest;
    double tx = 0.0;
    double ty = 0.0;

    str_init(&str);
    cairo_set_source_rgb(cr, 0, 0, 0);

    layout = pango_cairo_create_layout(cr);
    desc   = pango_font_description_from_string("Sans 16");
    pango_layout_set_font_description(layout, desc);

    csfg_poly_expr_to_str(&str, pool, tf->num);
    pango_layout_set_text(layout, str_cstr(str), -1);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);
    pango_layout_get_pixel_size(layout, &tw, &th);
    widest = tw;

    str_clear(str);
    csfg_poly_expr_to_str(&str, pool, tf->den);
    pango_layout_set_text(layout, str_cstr(str), -1);
    cairo_move_to(cr, tx, ty + th);
    pango_cairo_show_layout(cr, layout);
    pango_layout_get_pixel_size(layout, &tw, &th);
    widest = tw > widest ? tw : widest;

    cairo_move_to(cr, 0.0, th + 1.0);
    cairo_line_to(cr, widest, th + 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    g_object_unref(layout);
    pango_font_description_free(desc);
    str_deinit(str);
}

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t* cr,
    int width,
    int height,
    gpointer user_data)
{
    MathViewer* viewer = user_data;
    (void)area, (void)width, (void)height;

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    for (int x = -1000; x < 1000; x += 20)
    {
        cairo_move_to(cr, x, -1000);
        cairo_line_to(cr, x, 1000);
    }
    for (int y = -1000; y < 1000; y += 20)
    {
        cairo_move_to(cr, -1000, y);
        cairo_line_to(cr, 1000, y);
    }
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 32);
    struct csfg_layout layout;
    struct csfg_expr_pool* pool;
    csfg_layout_vec_init(&layout.nodes);
    str_init(&layout.str);
    csfg_expr_pool_init(&pool);
    int expr = csfg_expr_parse(&pool, cstr_view("A / (G1+G2) * s"));
    int n    = csfg_expr_layout(&layout, pool, expr, cr);
    csfg_expr_draw(&layout, pool, n, 0, 30, cr);
    csfg_expr_pool_deinit(pool);
    str_deinit(layout.str);
    csfg_layout_vec_deinit(layout.nodes);

    /*
    if (viewer->pool != NULL && viewer->expr > -1)
        draw_expr(cr, viewer->pool, viewer->expr);
    if (viewer->pool != NULL && viewer->tf != NULL)
        draw_tf(cr, viewer->pool, viewer->tf);*/
}

/* -------------------------------------------------------------------------- */
static void math_viewer_init(MathViewer* self)
{
    self->pool = NULL;
    self->pool = NULL;
    self->expr = -1;
    self->expr = -1;

    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_size_request(self->drawing_area, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_cb, self, NULL);

    gtk_box_append(GTK_BOX(self), self->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_init(MathViewerClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_finalize(MathViewerClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void math_viewer_register_type_internal(GTypeModule* type_module)
{
    math_viewer_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
MathViewer* math_viewer_new(void)
{
    return g_object_new(PLUGIN_TYPE_MATH_VIEWER, NULL);
}

/* -------------------------------------------------------------------------- */
void math_viewer_set_expr(
    MathViewer* viewer, const struct csfg_expr_pool* pool, int expr)
{
    viewer->tf   = NULL;
    viewer->pool = pool;
    viewer->expr = expr;
    gtk_widget_queue_draw(viewer->drawing_area);
}

/* -------------------------------------------------------------------------- */
void math_viewer_set_tf(
    MathViewer* viewer,
    const struct csfg_expr_pool* pool,
    const struct csfg_tf_expr* tf)
{
    viewer->tf   = tf;
    viewer->pool = pool;
    viewer->expr = -1;
    gtk_widget_queue_draw(viewer->drawing_area);
}
