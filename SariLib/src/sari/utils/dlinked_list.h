#pragma once

#include <functional>

namespace Sari { namespace Utils {

    template<typename Data>
    class DLinkedList {
    public:

        class Element {
            friend class DLinkedList;
        public:

            Element(Data data) :
                list_(nullptr),
                prev_(nullptr),
                next_(nullptr),
                data_(std::move(data))
            {}

            ~Element()
            {
                if (isStored()) {
                    list_->remove(this);
                }
            }

            bool isStored() const { return list_ != nullptr; };
            Data& data() { return data_; }

        private:

            DLinkedList* list_;

            class Element* prev_;
            class Element* next_;

            Data data_;

        };

        using ForcedRemovalHandler = std::function<void(Element*)>;

        DLinkedList()
        {}

        // ForcedRemovalHandler is called during list destruction
        // for each element that has not been removed.
        DLinkedList(ForcedRemovalHandler forcedRemovalHandler) :
            forcedRemovalHandler_(forcedRemovalHandler)
        {}

        ~DLinkedList()
        {
            Element* curr = first();

            while (curr) {

                Element* next = curr->next_;

                remove(curr);

                if (forcedRemovalHandler_) {
                    forcedRemovalHandler_(curr);
                }

                curr = next;
            }
        }

        Element* first() { return first_; }
        Element* last() { return last_; }

        void append(Element* e)
        {
            e->list_ = this;
        
            if (first_) {
                e->prev_ = last_;
                last_->next_ = e;
                last_ = e;
            }
            else {
                first_ = e;
                last_ = e;
            }
        }

        void remove(Element* e)
        {
            Element* prev = e->prev_;
            Element* next = e->next_;

            if (e->prev_) {
                e->prev_->next_ = next;
            }
            else {
                first_ = next;
            }

            if (e->next_) {
                e->next_->prev_ = prev;
            }
            else {
                last_ = prev;
            }

            e->list_ = nullptr;
            e->prev_ = nullptr;
            e->next_ = nullptr;
        }

    private:

        Element* first_ = nullptr;
        Element* last_ = nullptr;

        ForcedRemovalHandler forcedRemovalHandler_;

    };

}}