#define FOV 1.0

// More stuff on SDFs:
// https://iquilezles.org/articles/distfunctions

vec2 cmul(vec2 z1, vec2 z2) {
    // (a+bi)*(c+di)
    // (ac - bd) + i(ad + bc)
    return vec2(
        z1.x+z2.x - z1.y*z2.y,
        z1.x*z2.y + z1.y*z2.x
    );
}

vec2 cdiv(vec2 z1, vec2 z2) {
    // (a+bi)/(c+di)
    // (ac+bd)/(c^2+d^2) + i((bc-ad)/(c^2+d^2))
    float d = dot(z2, z2);
    return vec2(
        dot(z1, z2) / d,
        (z1.y*z2.x - z1.x*z2.y) / d
    );
}

vec2 BP2(vec2 s, float wp, float wq) {
    //wp*s / (s^2 + wp/wq*s + wp^2)
    vec2 num = wp*s;
    vec2 den = cmul(s, s) + wp/wq*s + vec2(wq*wq, 0.0);
    return cdiv(num, den);
}

vec2 LP1(vec2 s, float wp)
{
    // wp/(s+wp)
    vec2 num = vec2(wp, 0.0);
    vec2 den = s + vec2(wp, 0.0);
    return cdiv(num, den);
}

vec2 HP1(vec2 s, float wp)
{
    // s/(s+wp)
    vec2 num = s;
    vec2 den = s + vec2(wp, 0.0);
    return cdiv(num, den);
}

float surfaceHeightHP1(vec2 s, float wp) {
    float h = log(length(HP1(s, wp)));
    return clamp(h, -5.0, 5.0);
}

float sdfHP1(vec3 p, float wp) {
    vec2 s = p.xz;
    float h = surfaceHeightHP1(s, wp);
    
    // Numerical gradient (finite differences)
    float eps = 0.01;
    float hx = (surfaceHeightHP1(s + vec2(eps,0), wp) - h) / eps;
    float hy = (surfaceHeightHP1(s + vec2(0,eps), wp) - h) / eps;

    float den = sqrt(1.0 + hx*hx + hy*hy);
    return (p.y - h) / den;
}


float sdfBox(vec3 p, vec3 s) {
    vec3 d = abs(p) - s;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdfSphere(vec3 c, float r) {
    return length(c) - r;
}

float opUnion(float d1, float d2) {
    return min(d1, d2);
}
float opUnionSmooth(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5*(d2-d1)/k, 0.0, 1.0);
    return mix(d2, d1, h) - k*h*(1.-h);
}
float opSubtraction(float d1, float d2) {
    return max(-d1, d2);
}
float opSubtractionSmooth(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5*(d2+d1)/k, 0.0, 1.0);
    return mix(d2, -d1, h) + k*h*(1.0-h);
}
float opIntersection(float d1, float d2) {
    return max(d1, d2);
}
float opIntersectionSmooth(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5*(d2-d1)/k, 0.0, 1.0);
    return mix(d2, d1, h) + k*h*(1.0-h);
}

mat2 rot2D(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}
vec3 rot3D(vec3 p, vec3 axis, float angle) {
    // Rodrigues' rotation Formula
    return mix(dot(axis, p) * axis, p, cos(angle))
        + cross(axis, p) * sin(angle);
}

float sdfOld(vec3 p) {
    vec3 spherePos = vec3(sin(iTime)*3., 0, 0);
    float sphere = sdfSphere(p - spherePos, 1.);
    
    vec3 boxPos = vec3(0, 0, 0);
    vec3 q = rot3D(p, normalize(vec3(1, 1, 1)), iTime*0.4);
    float box = sdfBox(q - boxPos, vec3(.6));
    
    float ground = p.y + 1.1;
    
    float sdf = opUnionSmooth(sphere, box, .5);
    sdf = opUnion(sdf, ground);
    return sdf;
}

float sdf(vec3 p) {
    return sdfHP1(p, 1.0);
}

// https://iquilezles.org/articles/normalsSDF
vec3 normal(vec3 pos)
{
    vec2 e = vec2(1.0,-1.0)*0.5773;
    const float eps = 0.0005;
    return normalize(
        e.xyy*sdf(pos + e.xyy*eps) + 
        e.yyx*sdf(pos + e.yyx*eps) + 
        e.yxy*sdf(pos + e.yxy*eps) + 
        e.xxx*sdf(pos + e.xxx*eps)
    );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = (fragCoord * 2. - iResolution.xy) / iResolution.y;
    vec2 m = (iMouse.xy * 2. - iResolution.xy) / iResolution.y;

    // Initialization
    vec3 ro = vec3(0, 0, -3);             // Ray origin
    vec3 rd = normalize(vec3(uv*FOV, 1)); // Ray direction
    float t = 0.;                         // total distance travelled
    
    ro.yz *= rot2D(-m.y+.5);
    rd.yz *= rot2D(-m.y+.5);
    ro.xz *= rot2D(-m.x);
    rd.xz *= rot2D(-m.x);
    
    // raymarching
    for (int i = 0; i < 80; i++) {
        vec3 p = ro + rd * t;  // position along the ray
        float d = sdf(p);
        t += d;
        
        if (d < .001) break;
        if (t > 100.) break;
    }
    
    // coloring
    //vec3 col = vec3(t * .2);
    vec3 p = ro + rd * t;
    vec3 n = normal(p);
    float dif = clamp(dot(n, vec3(0.57703)), 0.0, 1.0);
    float amb = 0.5 + 0.5*dot(n, vec3(0, 1, 0));
    vec3 col = vec3(0.2, 0.3, 0.4) * amb + vec3(0.8, 0.7, 0.5)*dif;
    col += vec3(t*.01);
    
    //vec2 z = BP2(uv, .2, 1.0);
    vec2 z = HP1(uv, 1.0);
    float a = 0.5+.2*log(length(z));
    //col = vec3(a);

    // Output to screen
    fragColor = vec4(col, 1);
}
