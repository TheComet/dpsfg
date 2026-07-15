#pragma once

#include "csfg/util/vec.h"

struct serializer;
struct deserializer;
struct graph_model;

VEC_DECLARE(undo_stack_vec, struct serializer*, 32)

int undo_push_state(struct graph_model* model);
int undo(struct graph_model* model);
int redo(struct graph_model* model);
int undo_save_stack(struct serializer** ser, const struct graph_model* model);
int undo_load_stack(struct deserializer* des, struct graph_model* model);
void undo_clear_stack(struct graph_model* model);
void undo_reinit_stack(struct graph_model* model);
