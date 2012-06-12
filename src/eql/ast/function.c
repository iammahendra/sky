#include <stdlib.h>
#include <stdbool.h>
#include "../../dbg.h"

#include "node.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

//==============================================================================
//
// Functions
//
//==============================================================================

// Creates an AST node for a function.
//
// name        - The name of the function.
// return_type - The data type that the function returns.
// args        - The arguments of the function.
// arg_count   - The number of arguments the function has.
// body        - The contents of the function.
// ret         - A pointer to where the ast node will be returned.
//
// Returns 0 if successful, otherwise returns -1.
int eql_ast_function_create(bstring name, bstring return_type,
                            struct eql_ast_node **args, unsigned int arg_count,
                            struct eql_ast_node *body,
                            struct eql_ast_node **ret)
{
    eql_ast_node *node = malloc(sizeof(eql_ast_node)); check_mem(node);
    node->type = EQL_AST_TYPE_FUNCTION;
    node->function.name = bstrcpy(name);
    if(name) check_mem(node->function.name);
    node->function.return_type = bstrcpy(return_type);
    if(return_type) check_mem(node->function.return_type);

    // Copy arguments.
    if(arg_count > 0) {
        size_t sz = sizeof(eql_ast_node*) * arg_count;
        node->function.args = malloc(sz);
        check_mem(node->function.args);
        memcpy(node->function.args, args, sz);
    }
    else {
        node->function.args = NULL;
    }
    node->function.arg_count = arg_count;
    
    // Assign function body.
    node->function.body = body;

    *ret = node;
    return 0;

error:
    eql_ast_node_free(node);
    (*ret) = NULL;
    return -1;
}

// Frees a variable declaration AST node from memory.
//
// node - The AST node to free.
void eql_ast_function_free(struct eql_ast_node *node)
{
    if(node->function.name) bdestroy(node->function.name);
    node->function.name = NULL;

    if(node->function.return_type) bdestroy(node->function.return_type);
    node->function.return_type = NULL;
    
    if(node->function.arg_count > 0) {
        unsigned int i;
        for(i=0; i<node->function.arg_count; i++) {
            eql_ast_node_free(node->function.args[i]);
            node->function.args[i] = NULL;
        }
        free(node->function.args);
        node->function.arg_count = 0;
    }

    if(node->function.body) eql_ast_node_free(node->function.body);
    node->function.body = NULL;
}


//--------------------------------------
// Codegen
//--------------------------------------

// Recursively generates LLVM code for the function AST node.
//
// node    - The node to generate an LLVM value for.
// module  - The compilation unit this node is a part of.
// value   - A pointer to where the LLVM value should be returned.
//
// Returns 0 if successful, otherwise returns -1.
int eql_ast_function_codegen(eql_ast_node *node, eql_module *module,
                             LLVMValueRef *value)
{
    int rc;
    unsigned int i;
    
    LLVMContextRef context = LLVMGetModuleContext(module->llvm_module);
    
    // Create a list of function argument types.
    eql_ast_node *arg;
    unsigned int arg_count = node->function.arg_count;
    LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * arg_count);
    for(i=0; i<arg_count; i++) {
        arg = node->function.args[i];
        rc = eql_ast_farg_typegen(arg, module, &params[i]);
        check(rc == 0, "Unable to determine function argument type");
    }

    // Create function type.
    LLVMTypeRef funcType = LLVMFunctionType(LLVMVoidTypeInContext(context), params, arg_count, false);
    check(funcType != NULL, "Unable to create function type");

    // Create function.
    LLVMValueRef func = LLVMAddFunction(module->llvm_module, bdata(node->function.name), funcType);
    check(func != NULL, "Unable to create function");
    
    // Store the current function on the module.
    module->llvm_function = func;

    // Assign names to function arguments.
    for(i=0; i<arg_count; i++) {
        arg = node->function.args[i];
        LLVMValueRef param = LLVMGetParam(func, i);
        LLVMSetValueName(param, bdata(arg->farg.var_decl->var_decl.name));
    }

    // Generate body.
    LLVMValueRef body;
    rc = eql_ast_node_codegen(node->function.body, module, &body);
    check(rc == 0, "Unable to generate function body");
    
    // Dump before verification.
    // LLVMDumpValue(func);
    
    // Verify function.
    rc = LLVMVerifyFunction(func, LLVMPrintMessageAction);
    check(rc != 1, "Invalid function");

    // Unset the current function.
    module->llvm_function = NULL;

    // Return function as a value.
    *value = func;
    
    return 0;

error:
    // Unset the current function.
    module->llvm_function = NULL;

    if(func) LLVMDeleteFunction(func);
    *value = NULL;
    return -1;
}


//--------------------------------------
// Misc
//--------------------------------------

// Updates the return type of the function based on the last return statement
// of the function. This is used for implicit functions like the main function
// of a module.
//
// node - The function ast node to generate a type for.
//
// Returns 0 if successful, otherwise returns -1.
int eql_ast_function_generate_return_type(eql_ast_node *node)
{
    int rc;
    bstring type;
    
    check(node != NULL, "Function required");
    check(node->type == EQL_AST_TYPE_FUNCTION, "Node type must be 'function'");
    
    // If function has no body then its return type is void.
    eql_ast_node *body = node->function.body;
    if(body == NULL) {
        type = bfromcstr("void");
    }
    // Otherwise find the last return statement and determine its type.
    else {
        eql_ast_node *freturn = NULL;
        
        // Loop over all returns and save the last one.
        unsigned int i;
        for(i=0; i<body->block.expr_count; i++) {
            if(body->block.exprs[i]->type == EQL_AST_TYPE_FRETURN) {
                freturn = body->block.exprs[i];
            }
        }
        
        // If there is no return statement or it's a void return then the type
        // is void.
        if(freturn == NULL || freturn->freturn.value == NULL) {
            type = bfromcstr("void");
        }
        // Otherwise check the last return value to determine its type.
        else {
            rc = eql_ast_node_get_type(freturn->freturn.value, &type);
            check(rc == 0, "Unable to determine return type");
        }
    }
    
    // Assign type to return type.
    node->function.return_type = type;
    
    return 0;
    
error:
    bdestroy(type);
    return -1;
}