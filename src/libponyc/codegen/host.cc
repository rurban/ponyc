#ifdef _MSC_VER
#  pragma warning(push)
//because LLVM IR Builder code is broken: e.g. Instructions.h:521-527
#  pragma warning(disable:4244)
#  pragma warning(disable:4800)
#  pragma warning(disable:4267)
#  pragma warning(disable:4624)
#  pragma warning(disable:4141)
#  pragma warning(disable:4291)
#  pragma warning(disable:4146)
#endif

#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 0

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/SourceMgr.h>

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#include <stdio.h>
#include "codegen.h"
#include "ponyassert.h"

using namespace llvm;

char* LLVMGetHostCPUName()
{
  return strdup(sys::getHostCPUName().str().c_str());
}

char* LLVMGetHostCPUFeatures()
{
  StringMap<bool> features;
  bool got_features = sys::getHostCPUFeatures(features);
  pony_assert(got_features);
  (void)got_features;

  // Calculate the size of buffer that will be needed to return all features.
  size_t buf_size = 0;
  for(auto it = features.begin(); it != features.end(); it++)
    buf_size += (*it).getKey().str().length() + 2; // plus +/- char and ,/null

  char* buf = (char*)malloc(buf_size);
  pony_assert(buf != NULL);
  buf[0] = 0;

  for(auto it = features.begin(); it != features.end();)
  {
    if((*it).getValue())
      strcat(buf, "+");
    else
      strcat(buf, "-");

    strcat(buf, (*it).getKey().str().c_str());

    it++;
    if(it != features.end())
      strcat(buf, ",");
  }

  return buf;
}

void LLVMSetUnsafeAlgebra(LLVMValueRef inst)
{
  unwrap<Instruction>(inst)->setHasUnsafeAlgebra(true);
}

void LLVMSetNoUnsignedWrap(LLVMValueRef inst)
{
  unwrap<BinaryOperator>(inst)->setHasNoUnsignedWrap(true);
}

void LLVMSetNoSignedWrap(LLVMValueRef inst)
{
  unwrap<BinaryOperator>(inst)->setHasNoSignedWrap(true);
}

void LLVMSetIsExact(LLVMValueRef inst)
{
  unwrap<BinaryOperator>(inst)->setIsExact(true);
}

#if PONY_LLVM < 309
void LLVMSetReturnNoAlias(LLVMValueRef fun)
{
  unwrap<Function>(fun)->setDoesNotAlias(0);
}

void LLVMSetDereferenceable(LLVMValueRef fun, uint32_t i, size_t size)
{
  Function* f = unwrap<Function>(fun);

  AttrBuilder attr;
  attr.addDereferenceableAttr(size);

  f->addAttributes(i, AttributeSet::get(f->getContext(), i, attr));
}

void LLVMSetDereferenceableOrNull(LLVMValueRef fun, uint32_t i, size_t size)
{
  Function* f = unwrap<Function>(fun);

  AttrBuilder attr;
  attr.addDereferenceableOrNullAttr(size);

  f->addAttributes(i, AttributeSet::get(f->getContext(), i, attr));
}

#  if PONY_LLVM >= 308
void LLVMSetCallInaccessibleMemOnly(LLVMValueRef inst)
{
  Instruction* i = unwrap<Instruction>(inst);
  if(CallInst* c = dyn_cast<CallInst>(i))
    c->addAttribute(AttributeSet::FunctionIndex,
      Attribute::InaccessibleMemOnly);
  else if(InvokeInst* c = dyn_cast<InvokeInst>(i))
    c->addAttribute(AttributeSet::FunctionIndex,
      Attribute::InaccessibleMemOnly);
  else
    pony_assert(0);
}

void LLVMSetInaccessibleMemOrArgMemOnly(LLVMValueRef fun)
{
  unwrap<Function>(fun)->setOnlyAccessesInaccessibleMemOrArgMem();
}

void LLVMSetCallInaccessibleMemOrArgMemOnly(LLVMValueRef inst)
{
  Instruction* i = unwrap<Instruction>(inst);
  if(CallInst* c = dyn_cast<CallInst>(i))
    c->addAttribute(AttributeSet::FunctionIndex,
      Attribute::InaccessibleMemOrArgMemOnly);
  else if(InvokeInst* c = dyn_cast<InvokeInst>(i))
    c->addAttribute(AttributeSet::FunctionIndex,
      Attribute::InaccessibleMemOrArgMemOnly);
  else
    pony_assert(0);
}
#  endif
#endif

LLVMValueRef LLVMConstNaN(LLVMTypeRef type)
{
  Type* t = unwrap<Type>(type);

  Value* nan = ConstantFP::getNaN(t);
  return wrap(nan);
}

LLVMValueRef LLVMConstInf(LLVMTypeRef type, bool negative)
{
  Type* t = unwrap<Type>(type);

#if PONY_LLVM >= 307
  Value* inf = ConstantFP::getInfinity(t, negative);
#else
  fltSemantics const& sem = *float_semantics(t->getScalarType());
  APFloat flt = APFloat::getInf(sem, negative);
  Value* inf = ConstantFP::get(t->getContext(), flt);
#endif

  return wrap(inf);
}

#if PONY_LLVM < 308
static LLVMContext* unwrap(LLVMContextRef ctx)
{
  return reinterpret_cast<LLVMContext*>(ctx);
}
#endif

LLVMModuleRef LLVMParseIRFileInContext(LLVMContextRef ctx, const char* file)
{
  SMDiagnostic diag;

  return wrap(parseIRFile(file, diag, *unwrap(ctx)).release());
}

// From LLVM internals.
static MDNode* extractMDNode(MetadataAsValue* mdv)
{
  Metadata* md = mdv->getMetadata();
  pony_assert(isa<MDNode>(md) || isa<ConstantAsMetadata>(md));

  MDNode* n = dyn_cast<MDNode>(md);
  if(n != NULL)
    return n;

  return MDNode::get(mdv->getContext(), md);
}

void LLVMSetMetadataStr(LLVMValueRef inst, const char* str, LLVMValueRef node)
{
  pony_assert(node != NULL);

  MDNode* n = extractMDNode(unwrap<MetadataAsValue>(node));

  unwrap<Instruction>(inst)->setMetadata(str, n);
}

void LLVMMDNodeReplaceOperand(LLVMValueRef parent, unsigned i, 
  LLVMValueRef node)
{
  pony_assert(parent != NULL);
  pony_assert(node != NULL);
  pony_assert(i >= 0);

  MDNode *pn = extractMDNode(unwrap<MetadataAsValue>(parent));
  MDNode *cn = extractMDNode(unwrap<MetadataAsValue>(node));
  pn->replaceOperandWith(i, cn);
}

LLVMValueRef LLVMMemcpy(LLVMModuleRef module, bool ilp32)
{
  Module* m = unwrap(module);

  Type* params[3];
  params[0] = Type::getInt8PtrTy(m->getContext());
  params[1] = params[0];
  params[2] = Type::getIntNTy(m->getContext(), ilp32 ? 32 : 64);

  return wrap(Intrinsic::getDeclaration(m, Intrinsic::memcpy, {params, 3}));
}

LLVMValueRef LLVMMemmove(LLVMModuleRef module, bool ilp32)
{
  Module* m = unwrap(module);

  Type* params[3];
  params[0] = Type::getInt8PtrTy(m->getContext());
  params[1] = params[0];
  params[2] = Type::getIntNTy(m->getContext(), ilp32 ? 32 : 64);

  return wrap(Intrinsic::getDeclaration(m, Intrinsic::memmove, {params, 3}));
}

LLVMValueRef LLVMLifetimeStart(LLVMModuleRef module)
{
  Module* m = unwrap(module);

  return wrap(Intrinsic::getDeclaration(m, Intrinsic::lifetime_start, {}));
}

LLVMValueRef LLVMLifetimeEnd(LLVMModuleRef module)
{
  Module* m = unwrap(module);

  return wrap(Intrinsic::getDeclaration(m, Intrinsic::lifetime_end, {}));
}
