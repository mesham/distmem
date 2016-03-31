/*
 * distmem.c
 *
 *  Created on: 3 Nov 2015
 *      Author: nick
 */

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <memkind/internal/memkind_arena.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <numa.h>
#include <math.h>
#include "distmem.h"

struct dist_intenal_distmem_node {
  struct distmem *item;
  struct dist_intenal_distmem_node *next;
};

const size_t ERROR_MESSAGE_SIZE = 1024;

struct dist_intenal_distmem_node **dist_memory_kinds = NULL;

static void remove_info_entry_from_state(struct distmem *, void *);
static int get_hashkey(void *);
static int get_hashkey_from_string(char *);
static void put_kind_entry_into_kind_state(struct distmem *);

void init_distmem() {
  dist_memory_kinds =
      (struct dist_intenal_distmem_node **)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct dist_intenal_distmem_node *) * HASH_SIZE);
  int i;
  for (i = 0; i < HASH_SIZE; i++) {
    dist_memory_kinds[i] = NULL;
  }
}

int distmem_create_default(struct distmem_ops *ops, const char *name, memkind_t *kind) {
  ops->memkind_operations = (struct memkind_ops *)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct memkind_ops));
  ops->memkind_operations->create = memkind_arena_create;
  ops->memkind_operations->destroy = memkind_arena_destroy;
  ops->memkind_operations->malloc = memkind_arena_malloc;
  ops->memkind_operations->calloc = memkind_arena_calloc;
  ops->memkind_operations->posix_memalign = memkind_arena_posix_memalign;
  ops->memkind_operations->realloc = memkind_arena_realloc;
  ops->memkind_operations->free = distmem_free;
  ops->memkind_operations->get_size = memkind_default_get_size;
  ops->memkind_operations->get_arena = memkind_thread_get_arena;
  return distmem_create(ops, name, kind);
}

int distmem_create(struct distmem_ops *ops, const char *name, memkind_t *kind) {
  int err = memkind_create(ops->memkind_operations, name, kind);
  if (err) {
    char err_msg[ERROR_MESSAGE_SIZE];
    memkind_error_message(err, err_msg, ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s", err_msg);
  }
  struct distmem *dist_kind = (struct distmem *)malloc(sizeof(struct distmem));
  dist_kind->memkind = *kind;
  dist_kind->name = (char *)malloc(strlen(name) + 1);
  dist_kind->operations = ops;
  strcpy(dist_kind->name, name);
  err = ops->dist_create(dist_kind, ops, name);
  if (err) {
    fprintf(stderr, "Error in dist memory creation\n");
  }
  put_kind_entry_into_kind_state(dist_kind);
  return 0;
}

struct distmem *distmem_get_distkind_by_name(char *name) {
  int hash_key = get_hashkey_from_string(name);
  struct dist_intenal_distmem_node *head = dist_memory_kinds[hash_key];
  while (head != NULL) {
    if (strcmp(head->item->name, name) == 0) return head->item;
    head = head->next;
  }
  return NULL;
}

int distmem_arena_create(struct distmem *dist_kind, struct distmem_ops *ops, const char *name) {
  int i;
  dist_kind->internal_state = (struct distmem_memory_information_generic **)memkind_malloc(
      MEMKIND_DEFAULT, sizeof(struct distmem_memory_information_generic *) * HASH_SIZE);
  for (i = 0; i < HASH_SIZE; i++) {
    dist_kind->internal_state[i] = NULL;
  }
  return 0;
}

void distmem_put_specific_entry_into_state(struct distmem *dist_kind, void *specific_info, void *ptr,
                                           void (*deallocate_specific_information)(void *)) {
  int hashkey = get_hashkey(ptr);
  struct distmem_memory_information_generic *generic_info =
      (struct distmem_memory_information_generic *)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct distmem_memory_information_generic));
  generic_info->ptr = ptr;
  generic_info->specific_information = specific_info;
  generic_info->deallocate_specific_information = deallocate_specific_information;
  generic_info->next = dist_kind->internal_state[hashkey];
  dist_kind->internal_state[hashkey] = generic_info;
}

void *distmem_get_specific_entry(struct distmem *dist_kind, void *ptr) {
  struct distmem_memory_information_generic *head = dist_kind->internal_state[get_hashkey(ptr)];
  while (head != NULL) {
    if (head->ptr == ptr) return head->specific_information;
    head = head->next;
  }
  return NULL;
}

static void remove_info_entry_from_state(struct distmem *dist_kind, void *ptr) {
  int hash_key = get_hashkey(ptr);
  struct distmem_memory_information_generic *specific_state = dist_kind->internal_state[hash_key], *last_entry = NULL;
  while (specific_state != NULL) {
    if (specific_state->ptr == ptr) {
      if (last_entry == NULL) {
        dist_kind->internal_state[hash_key] = specific_state->next;
      } else {
        last_entry->next = specific_state->next;
      }
      if (specific_state->deallocate_specific_information != NULL) {
        specific_state->deallocate_specific_information(specific_state->specific_information);
      } else {
        if (specific_state->specific_information != NULL) {
          memkind_free(MEMKIND_DEFAULT, specific_state->specific_information);
        }
      }
      memkind_free(MEMKIND_DEFAULT, specific_state);
      break;
    }
    last_entry = specific_state;
    specific_state = specific_state->next;
  }
}

void distmem_free(struct memkind *kind, void *ptr) {
  remove_info_entry_from_state(distmem_get_distkind_by_name(kind->name), ptr);
  memkind_free(MEMKIND_DEFAULT, ptr);
}

static void put_kind_entry_into_kind_state(struct distmem *dist_kind) {
  int hash_key = get_hashkey_from_string(dist_kind->name);
  struct dist_intenal_distmem_node *specific_node =
      (struct dist_intenal_distmem_node *)memkind_malloc(MEMKIND_DEFAULT, sizeof(struct dist_intenal_distmem_node));
  specific_node->item = dist_kind;
  specific_node->next = dist_memory_kinds[hash_key];
  dist_memory_kinds[hash_key] = specific_node;
}

static int get_hashkey_from_string(char *name) {
  int i, hashkey = 5381;
  for (i = 0; i < strlen(name); i++) {
    hashkey = (hashkey << 5 + hashkey) + (int)name[i];
  }
  return abs(hashkey % HASH_SIZE);
}

static int get_hashkey(void *buffer) { return ((uintptr_t)buffer) % HASH_SIZE; }
