#ifndef CODEGEN_GENTYPE_H
#define CODEGEN_GENTYPE_H

#include <platform.h>
#include "codegen.h"

PONY_EXTERN_C_BEGIN

#if PONY_LLVM >= 400

typedef struct tbaa_descriptor_t
{
  const char* name;
  reach_type_t* type;

  LLVMValueRef metadata;
  LLVMValueRef *field_offsets;
} tbaa_descriptor_t;

DECLARE_HASHMAP(tbaa_descriptors, tbaa_descriptors_t, tbaa_descriptor_t);
tbaa_descriptors_t* tbaa_descriptors_new();
void tbaa_descriptors_free(tbaa_descriptors_t* tbaa_descriptors);

typedef struct tbaa_access_tag_t
{
  const char* base_name;
  uint32_t field_index;
  const char* field_name;  
  LLVMValueRef metadata;
} tbaa_access_tag_t;

DECLARE_HASHMAP(tbaa_access_tags, tbaa_access_tags_t, tbaa_access_tag_t);
tbaa_access_tags_t* tbaa_access_tags_new();
void tbaa_access_tags_free(tbaa_access_tags_t* tbaa_access_tags);

void tbaa_tag_struct_access(compile_t* c, reach_type_t* base_type, 
  reach_type_t* field_type, uint32_t index, LLVMValueRef instr);

void tbaa_tag_struct_access_ast(compile_t *c, ast_t* base_type, 
  uint32_t index, LLVMValueRef instr);

void tbaa_tag_box_access(compile_t *c, reach_type_t* type, LLVMValueRef instr);

void tbaa_tag_scalar_access(compile_t* c, reach_type_t* type, 
  LLVMValueRef instr);
void tbaa_tag_scalar_access_ast(compile_t *c, ast_t* type, LLVMValueRef instr);

void get_fieldinfo(ast_t* l_type, ast_t* right, ast_t** l_def,
  ast_t** field, uint32_t* index);

#else

typedef struct tbaa_metadata_t
{
  const char* name;
  LLVMValueRef metadata;
} tbaa_metadata_t;

DECLARE_HASHMAP(tbaa_metadatas, tbaa_metadatas_t, tbaa_metadata_t);

tbaa_metadatas_t* tbaa_metadatas_new();

void tbaa_metadatas_free(tbaa_metadatas_t* tbaa_metadatas);

LLVMValueRef tbaa_metadata_for_type(compile_t* c, ast_t* type);

LLVMValueRef tbaa_metadata_for_box_type(compile_t* c, const char* box_name);

#endif

bool gentypes(compile_t* c);

PONY_EXTERN_C_END

#endif
