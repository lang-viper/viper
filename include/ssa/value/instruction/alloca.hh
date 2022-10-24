#ifndef VIPER_SSA_INSTRUCTION_ALLOCA_HH
#define VIPER_SSA_INSTRUCTION_ALLOCA_HH
#include <ssa/value/instruction/instruction.hh>
#include <ssa/value/tmpValue.hh>
#include <memory>

namespace SSA
{
    class AllocaInst : public Instruction
    {
    friend class Builder;
    friend class Function;
    public:
        void Print(std::ostream& stream, int indent) const override;
        std::string GetID() const override;

        Codegen::Value* Emit(Codegen::Assembly& assembly) override;

        void Dispose() override;

        std::shared_ptr<Type> GetAllocatedType() const;
        std::shared_ptr<Type> GetType() const override;

    protected:
        AllocaInst(Module& module, std::shared_ptr<Type> allocatedType, const std::string& name = "");
        ~AllocaInst();
    
    private:
        TempValue* _name;
        Codegen::MemoryValue* _memory;
        int _offset;
        std::shared_ptr<Type> _allocatedType;
    };
}

#endif