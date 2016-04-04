#include "ebpfTable.h"
#include "ebpfType.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"

namespace EBPF {

class ActionTranslationVisitor : public CodeGenInspector {
 protected:
    const EBPFProgram*         program;
    const IR::ActionContainer* action;
    cstring                    valueName;

 public:
    ActionTranslationVisitor(CodeBuilder* builder, cstring valueName, const EBPFProgram* program):
            CodeGenInspector(builder, program->blockMap->typeMap), program(program),
            action(nullptr), valueName(valueName)
    { CHECK_NULL(program); }

    bool preorder(const IR::PathExpression* expression) {
        auto decl = program->refMap->getDeclaration(expression->path, true);
        if (decl->is<IR::Parameter>()) {
            auto param = decl->to<IR::Parameter>();
            bool isParam = action->parameters->getParameter(param->name) == param;
            if (isParam) {
                builder->append(valueName);
                builder->append("->u.");
                builder->append(action->name);
                builder->append(".");
            }
        }
        builder->append(expression->path->toString());
        return false;
    }

    bool preorder(const IR::ActionContainer* act) {
        action = act;
        setVecSep("\n", "\n");
        visit(action->body);
        doneVec();
        return false;
    }
};  // ActionTranslationVisitor

////////////////////////////////////////////////////////////////

EBPFTable::EBPFTable(const EBPFProgram* program, const IR::TableBlock* table) :
        EBPFTableBase(program, table->container->name.name), table(table) {
    cstring base = instanceName + "_defaultAction";
    defaultActionMapName = program->refMap->newName(base);

    base = table->container->name.name + "_actions";
    actionEnumName = program->refMap->newName(base);

    keyGenerator = table->container->getKey();
    actionList = table->container->getActionList();
}

void EBPFTable::emitKeyType(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("struct %s ", keyTypeName);
    builder->blockStart();

    unsigned fieldNumber = 0;
    for (auto c : *keyGenerator->keyElements) {
        auto type = program->typeMap->getType(c->expression);
        builder->emitIndent();
        auto ebpfType = EBPFTypeFactory::instance->create(type);
        ebpfType->declare(builder, cstring("field") + fieldNumber, false);
        builder->endOfStatement(true);

        auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
        auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
        if (matchType->name.name != P4::P4CoreLibrary::instance.exactMatch.name)
            ::error("Match of type %1% not supported", c->matchType);
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFTable::emitActionArguments(CodeBuilder* builder, const IR::ActionContainer* action,
                                    cstring name) {
    builder->emitIndent();
    builder->append("struct ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = EBPFTypeFactory::instance->create(p->type);
        type->declare(builder, p->name.name, false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

void EBPFTable::emitValueType(CodeBuilder* builder) {
    // create an enum with tags for all actions
    builder->emitIndent();
    builder->append("enum ");
    builder->append(actionEnumName);
    builder->spc();
    builder->blockStart();

    for (auto a : *actionList->actionList) {
        cstring name = a->name->path->name;
        builder->emitIndent();
        builder->append(name);
        builder->append(",");
        builder->newline();
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);

    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->appendFormat("struct %s ", valueTypeName);
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("enum %s action;", actionEnumName);
    builder->newline();

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    for (auto a : *actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->name->path, true);
        auto action = adecl->getNode()->to<IR::ActionContainer>();
        emitActionArguments(builder, action, a->name->path->name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFTable::emit(CodeBuilder* builder) {
    emitKeyType(builder);
    emitValueType(builder);

    auto impl = table->container->properties->getProperty(program->model.tableImplProperty.name);
    if (impl == nullptr) {
        ::error("Table %1% does not have an %2% property",
                table->container, program->model.tableImplProperty.name);
        return;
    }

    // Some type checking...
    if (!impl->value->is<IR::ExpressionValue>()) {
        ::error("%1%: Expected property to be an `extern` block", impl);
        return;
    }

    auto expr = impl->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::ConstructorCallExpression>()) {
        ::error("%1%: Expected property to be an `extern` block", impl);
        return;
    }

    auto block = table->getValue(expr);
    if (block == nullptr || !block->is<IR::ExternBlock>()) {
        ::error("%1%: Expected property to be an `extern` block", impl);
        return;
    }

    bool isHash;
    auto extBlock = block->to<IR::ExternBlock>();
    if (extBlock->type->name.name == program->model.array_table.name) {
        isHash = false;
    } else if (extBlock->type->name.name == program->model.hash_table.name) {
        isHash = true;
    } else {
        ::error("%1%: implementation must be one of %2% or %3%",
                impl, program->model.array_table.name, program->model.hash_table.name);
        return;
    }

    auto sz = extBlock->getParameterValue(program->model.array_table.size.name);
    if (sz == nullptr || !sz->is<IR::Constant>()) {
        ::error("Expected an integer argument for %1%; is the model corrupted?", expr);
        return;
    }
    auto cst = sz->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error("%1%: size too large", cst);
        return;
    }
    int size = cst->asInt();
    if (size <= 0) {
        ::error("%1%: negative size", cst);
        return;
    }

    builder->emitIndent();
    builder->target->emitTableDecl(builder, table->container->name.name, isHash,
                                   cstring("struct ") + keyTypeName,
                                   cstring("struct ") + valueTypeName, size);
    builder->target->emitTableDecl(builder, defaultActionMapName, false,
                                   program->arrayIndexType,
                                   cstring("struct ") + valueTypeName, 1);
}

void EBPFTable::createKey(CodeBuilder* builder, cstring keyName) {
    unsigned fieldNumber = 0;
    for (auto c : *keyGenerator->keyElements) {
        builder->emitIndent();
        builder->append(keyName);
        builder->append(".field");
        builder->append(fieldNumber);
        builder->append(" = ");

        CodeGenInspector visitor(builder, program->blockMap->typeMap);
        c->expression->apply(visitor);
        builder->endOfStatement(true);
    }
}

void EBPFTable::runAction(CodeBuilder* builder, cstring valueName) {
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName);
    builder->blockStart();

    for (auto a : *actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->name->path, true);
        auto action = adecl->getNode()->to<IR::ActionContainer>();
        builder->emitIndent();
        builder->appendFormat("case %s: ", action->name.name);
        builder->newline();
        builder->emitIndent();
        builder->blockStart();

        ActionTranslationVisitor visitor(builder, valueName, program);
        action->apply(visitor);
        builder->blockEnd(true);
        builder->emitIndent();
        builder->appendLine("break;");
    }

    builder->blockEnd(true);
}

////////////////////////////////////////////////////////////////

EBPFCounterTable::EBPFCounterTable(const EBPFProgram* program,
                                   const IR::ExternBlock* block, cstring name) :
        EBPFTableBase(program, name) {
    auto sz = block->getParameterValue(program->model.counterArray.size.name);
    if (sz == nullptr || !sz->is<IR::Constant>()) {
        ::error("Expected an integer argument for parameter %1% or %2%; is the model corrupted?",
                program->model.counterArray.size, name);
        return;
    }
    auto cst = sz->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error("%1%: size too large", cst);
        return;
    }
    size = cst->asInt();
    if (size <= 0) {
        ::error("%1%: negative size", cst);
        return;
    }

    auto sprs = block->getParameterValue(program->model.counterArray.sparse.name);
    if (sprs == nullptr || !sprs->is<IR::BoolLiteral>()) {
        ::error("Expected an integer argument for parameter %1% or %2%; is the model corrupted?",
                program->model.counterArray.sparse, name);
        return;
    }

    isHash = sprs->to<IR::BoolLiteral>()->value;
}

void EBPFCounterTable::emit(CodeBuilder* builder) {
    auto indexType = EBPFTypeFactory::instance->create(EBPFModel::counterIndexType);
    auto valueType = EBPFTypeFactory::instance->create(EBPFModel::counterValueType);
    keyTypeName = indexType->toString(builder->target);
    valueTypeName = valueType->toString(builder->target);
    builder->target->emitTableDecl(builder, dataMapName, isHash,
                                   keyTypeName, valueTypeName, size);
}

void EBPFCounterTable::emitCounterIncrement(CodeBuilder* builder,
                                            const IR::MethodCallExpression *expression) {
    cstring keyName = program->refMap->newName("key");
    cstring valueName = program->refMap->newName("value");

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->append("*");
    builder->append(valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->appendLine("init_val = 1;");

    builder->emitIndent();
    builder->append(keyTypeName);
    builder->spc();
    builder->append(keyName);
    builder->append(" = ");

    BUG_CHECK(expression->arguments->size() == 1, "Expected just 1 argument for %1%", expression);
    auto arg = expression->arguments->at(0);

    CodeGenInspector visitor(builder, program->blockMap->typeMap);
    arg->apply(visitor);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->target->emitTableLookup(builder, dataMapName, keyName, valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL)", valueName.c_str());
    builder->newline();
    builder->increaseIndent();
    builder->emitIndent();
    builder->appendFormat("__sync_fetch_and_add(%s, 1);", valueName.c_str());
    builder->newline();
    builder->decreaseIndent();

    builder->emitIndent();
    builder->appendLine("else");
    builder->increaseIndent();
    builder->emitIndent();
    builder->target->emitTableUpdate(builder, dataMapName, keyName, "init_val");
    builder->newline();
    builder->decreaseIndent();
}

void
EBPFCounterTable::emitMethodInvocation(CodeBuilder* builder, P4::ExternMethod* method) {
    if (method->method->name.name == program->model.counterArray.increment.name) {
        emitCounterIncrement(builder, method->expr);
        return;
    }
    ::error("%1%: Unexpected method for %2%", method->expr, program->model.counterArray.name);
}

}  // namespace EBPF
