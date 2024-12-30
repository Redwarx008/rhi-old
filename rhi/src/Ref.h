#pragma once

namespace rhi
{
    template <typename T>
    class Ref
    {
    public:
        Ref() : mValue(nullptr) {}
        Ref(T* value) : mValue(value)
        {
            mValue->AddRef();
        }
        ~Ref() { mValue->Release(); }
        T* operator->() const
        {
            return mValue;
        }
        void Attach(T* ptr)
        {
            if (mValue != nullptr)
            {
                mValue->Release();
            }
            mValue = ptr;
        }
        T* Detach()
        {
            T* ptr = mValue;
            mValue = nullptr;
            return ptr;
        }
    private:
        T* mValue;
    };
}