// This is an implementation of DiskList that does not actually store the
// linked list on disk.  Unlike a disk-based data structure, instead of
// using offsets to indicate where a node on disk is, it uses plain old
// pointers into memory.
//
// This implementation does reuse nodes for items removed from the list,
// so it won't ask for more storage unless it has no old storage that it
// can reuse.

#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
using namespace std;

const int MAX_DATA_LENGTH = 255;

class DiskList
{
  public:
    DiskList(const std::string& filename);
    bool push_front(const char* data);
    bool remove(const char* data);
    void printAll();

      // These public members are here only for measurement; they would
      // be removed if we were forbidden to add new public members.
    int numberOfCallsToNew() const { return m_numberOfCallsToNew; }
    int freeListSize() const
    {
        int len = 0;
        for (const Node* p = header.freeList; p != nullptr; p = p->next)
            len++;
        return len;
    }

  private:
    struct Node
    {
        Node* next;
        char data[MAX_DATA_LENGTH+1];
    };

    struct Header
    {
        Node* head;     // head of the list of nodes being used
        Node* freeList; // head of the list of nodes available for reuse
    };

    Header header;  // In a disk-based implementation, this would be at the
                    // start of the disk file.

    Node* acquireNode();

      // This data member is here only to support a measurement function.
    int m_numberOfCallsToNew;
};

DiskList::DiskList(const std::string&)
{
    header.head = nullptr;
    header.freeList = nullptr;

      // This statement is here only to support a measurement function.
    m_numberOfCallsToNew = 0;
}

bool DiskList::push_front(const char* data)
{
    if (strlen(data) > MAX_DATA_LENGTH)
        return false;

    Node* p = acquireNode();
    strcpy(p->data, data);
    p->next = header.head;
    header.head = p;
    return true;
}

bool DiskList::remove(const char* data)
{
    bool anyRemoved = false;

    Node* curr = header.head;
    Node* prev = nullptr;

    while (curr != nullptr)
    {
        if (strcmp(curr->data, data) != 0)
        {
            prev = curr;
            curr = curr->next;
        }
        else
        {
              // Unhook node from linked list
            Node* toBeRemoved = curr;
            curr = curr->next;
            if (prev == nullptr)
                header.head = curr;
            else
                prev->next = curr;

              // Add removed node to front of freeList
            toBeRemoved->next = header.freeList;
            header.freeList = toBeRemoved;

            anyRemoved = true;
        }
    }

    return anyRemoved;
}

void DiskList::printAll()
{
    for (const Node* p = header.head; p != nullptr; p = p->next)
        cout << p->data << endl;
}

DiskList::Node* DiskList::acquireNode()
{
      // Allocate new storage only if no old node is available for reuse.

    if (header.freeList == nullptr)
    {
          // This statement is here only to support a measurement function.
        m_numberOfCallsToNew++;

        return new Node;
    }

    Node* p = header.freeList;
    header.freeList = p->next;
    return p;
}

int main()
{
    DiskList x("nameDoesNotMatterForThisNonDiskImplementation"); 
    x.push_front("Fred");
    x.push_front("Lucy");
    x.push_front("Ethel");
    x.push_front("Ethel");
    x.push_front("Lucy");
    x.push_front("Fred");
    x.push_front("Ethel");
    x.push_front("Ricky");
    x.push_front("Lucy");
    x.remove("Lucy");
    x.push_front("Fred");
    x.push_front("Ricky");
    x.printAll();
    assert(x.numberOfCallsToNew() == 9);  // not 11
    assert(x.freeListSize() == 1);  // 3 Lucys removed, then 2 push_fronts
    cout << "Passed test if output above, one per line, is" << endl;
    cout << "Ricky Fred Ricky Ethel Fred Ethel Ethel Fred" << endl;
}
