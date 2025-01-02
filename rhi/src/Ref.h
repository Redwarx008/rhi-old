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
        Ref(Ref<T>&& other)
        {
            mValue = other.Detach();
        }
        Ref(const Ref<T>& other) : mValue(other.mValue)
        {
            other.mValue->AddRef();
        }
        ~Ref() { mValue->Release(); }

        Ref<T>& operator=(const Ref<T>& other) 
        {
            if (mValue != nullptr)
            {
                mValue->Release();
            }
            mValue = other->mValue;
            mValue->AddRef();
            return *this;
        }
        Ref<T>& operator=(T* other)
        {
            if (mValue != nullptr)
            {
                mValue->Release();
            }
            mValue = other;
            mValue->AddRef();
            return *this;
        }
        Ref<T>& operator=(Ref<T>&& other)
        {
            mValue = other.Detach();
            return *this;
        }

        bool operator==(const T* other) const
        {
            return mValue == other;
        }
        bool operator!=(const T* other) const
        {
            return mValue != other;
        }
        bool operator==(const Ref<T>& other) const
        {
            return mValue == other.mValue;
        }
        bool operator!=(const Ref<T>& other) const
        {
            return mValue != other.mValue;
        }

        T* operator->() const
        {
            return mValue;
        }


        // Set the pointer while keeping the object's reference count unchanged
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