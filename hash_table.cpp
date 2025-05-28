#include <iostream>
#include <cmath>
#include <string>
#include <iomanip>
#include <algorithm>

class Record
{
public:
    std::string fio;
    std::string carInfo;
    std::string time;
    int lineNumber;
    int number;

    Record() : fio(""), carInfo(""), time(""), number(0), lineNumber(-1) {}

    Record(std::string fio_, int number_) : fio(std::move(fio_)), carInfo(""), time(""), number(number_), lineNumber(-1) {}

    Record(std::string fio_, std::string carInfo_, std::string time_, int number_, int lineNumber_ = -1)
        : fio(std::move(fio_)), carInfo(std::move(carInfo_)), time(std::move(time_)), number(number_), lineNumber(lineNumber_) {}

    int getKey() const
    {
        int key = 0;
        for (char c : fio)
        {
            key += static_cast<int>(c);
        }
        return key + number;
    }

    bool compareKeys(const Record &record) const
    {
        return (fio == record.fio && number == record.number);
    }

    void printKey() const
    {
        std::cout << fio << " " << number;
    }

    void printRest() const
    {
        std::cout << " " << carInfo;
    }
};

template <typename T = Record>
int MidSquareHash(T record, int tableSize)
{
    int key = record.getKey();
    int squared = key * key;
    std::string squaredStr = std::to_string(squared);
    int len = squaredStr.size();
    int mid = len / 2;
    int sliceSize = std::to_string(tableSize).size();

    std::string middle = squaredStr.substr(std::max(0, mid - sliceSize / 2), sliceSize);
    if (middle.empty())
        return 0;

    return std::stoi(middle) % tableSize;
}

template <typename T = Record>
struct PrimaryHash
{
    int operator()(T record, int tableSize) const
    {
        return MidSquareHash(record, tableSize);
    }
};

struct QuadraticProbing
{
private:
    int c1 = 1;
    int c2 = 2;

public:
    int operator()(int hash, int j) const
    {
        return hash + c1 * j + c2 * j * j;
    }
};

template <typename T = Record, class Hash1 = PrimaryHash<T>, class Hash2 = QuadraticProbing>
class HashTable
{
public:
    class Node
    {
    public:
        T value;
        char status; // 0 = empty, 1 = occupied, 2 = deleted

        Node() : status(0) {}
        Node(T val) : value(std::move(val)), status(1) {}
        void disable()
        {
            value.lineNumber = -1;
            status = 2;
        }
    };

private:
    using ValueType = T;

    static const inline int DEFAULT_SIZE = 8;
    static constexpr double LOAD_FACTOR = 0.75;
    int bufferSize;
    int size;
    int usedSlots;
    Node **table;

    void initializeTable(Node **tbl, int size)
    {
        for (int i = 0; i < size; i++)
        {
            tbl[i] = nullptr;
        }
    }

    bool insert(ValueType value, bool rehash = false)
    {
        Hash1 h1;
        Hash2 h2;
        int initialIndex = h1(value, bufferSize);
        Node *newNode = new Node(std::move(value));

        if (table[initialIndex] == nullptr)
        {
            table[initialIndex] = newNode;
            size++;
            if (!rehash)
                usedSlots++;
            if (!rehash)
                checkForResize();
            return true;
        }
        else if (table[initialIndex]->status != 1)
        {
            delete table[initialIndex];
            table[initialIndex] = newNode;
            size++;
            if (!rehash)
                usedSlots++;
            if (!rehash)
                checkForResize();
            return true;
        }

        int j = 1;
        int probeIndex;
        do
        {
            probeIndex = h2(initialIndex, j) % bufferSize;

            if (table[probeIndex] == nullptr)
            {
                table[probeIndex] = newNode;
                size++;
                if (!rehash)
                    usedSlots++;
                if (!rehash)
                    checkForResize();
                return true;
            }
            else if (table[probeIndex]->status != 1)
            {
                delete table[probeIndex];
                table[probeIndex] = newNode;
                size++;
                if (!rehash)
                    usedSlots++;
                if (!rehash)
                    checkForResize();
                return true;
            }
            else if (table[probeIndex]->value.compareKeys(newNode->value))
            {
                delete newNode;
                return false;
            }

            j++;
        } while (j < bufferSize);

        delete newNode;
        return false;
    }

    void checkForResize()
    {
        double loadFactor = static_cast<double>(usedSlots) / bufferSize;

        if (loadFactor > LOAD_FACTOR ||
            (loadFactor < (1 - LOAD_FACTOR) && bufferSize > DEFAULT_SIZE))
        {
            int newSize = (loadFactor > LOAD_FACTOR) ? bufferSize * 2 : bufferSize / 2;
            newSize = std::max(newSize, DEFAULT_SIZE);

            Node **newTable = new Node *[newSize];
            initializeTable(newTable, newSize);

            int oldSize = bufferSize;
            Node **oldTable = table;

            table = newTable;
            bufferSize = newSize;
            size = 0;
            usedSlots = 0;

            for (int i = 0; i < oldSize; i++)
            {
                if (oldTable[i] != nullptr && oldTable[i]->status == 1)
                {
                    insert(oldTable[i]->value, true);
                }
                delete oldTable[i];
            }
            delete[] oldTable;
        }
    }

    Node *findNode(ValueType value)
    {
        Hash1 h1;
        Hash2 h2;
        int initialIndex = h1(value, bufferSize);

        if (table[initialIndex] != nullptr &&
            table[initialIndex]->status == 1 &&
            table[initialIndex]->value.compareKeys(value))
        {
            return table[initialIndex];
        }

        int j = 1;
        int probeIndex;
        while (j < bufferSize)
        {
            probeIndex = h2(initialIndex, j) % bufferSize;

            if (table[probeIndex] == nullptr)
                break;

            if (table[probeIndex]->status == 1 &&
                table[probeIndex]->value.compareKeys(value))
            {
                return table[probeIndex];
            }

            if (table[probeIndex]->status == 0)
                break;

            j++;
        }

        return nullptr;
    }

    bool deleteElement(Record recordToDelete)
    {
        Node *nodeToDelete = findNode(recordToDelete);

        if (nodeToDelete == nullptr || nodeToDelete->status != 1)
            return false;

        nodeToDelete->disable();
        usedSlots--;
        checkForResize();
        return true;
    }

public:
    HashTable() : bufferSize(DEFAULT_SIZE), size(0), usedSlots(0)
    {
        table = new Node *[DEFAULT_SIZE];
        initializeTable(table, DEFAULT_SIZE);
    }

    ~HashTable()
    {
        for (int i = 0; i < bufferSize; i++)
        {
            delete table[i];
        }
        delete[] table;
    }

    void print() const
    {
        std::cout << std::left;
        std::cout << std::setw(6) << "Index"
                  << std::setw(25) << "Key (FIO + Number)"
                  << std::setw(25) << "Car Info"
                  << std::setw(12) << "Status" << "\n";

        std::cout << std::string(68, '-') << "\n";

        for (int i = 0; i < bufferSize; ++i)
        {
            std::cout << std::setw(6) << i;

            if (table[i] == nullptr)
            {
                std::cout << std::setw(25) << "no data"
                          << std::setw(25) << "-"
                          << std::setw(12) << "empty";
            }
            else if (table[i]->status == 0)
            {
                std::cout << std::setw(25) << "no data"
                          << std::setw(25) << "-"
                          << std::setw(12) << "empty";
            }
            else if (table[i]->status == 1)
            {
                std::string keyStr = table[i]->value.fio + " " + std::to_string(table[i]->value.number);
                std::cout << std::setw(25) << keyStr
                          << std::setw(25) << table[i]->value.carInfo
                          << std::setw(12) << "occupied";
            }
            else // status == 2
            {
                std::cout << std::setw(25) << "deleted"
                          << std::setw(25) << "-"
                          << std::setw(12) << "deleted";
            }

            std::cout << "\n";
        }
        std::cout << "Buffer size: " << bufferSize
                  << ", Used slots: " << usedSlots
                  << ", Load factor: " << std::fixed << std::setprecision(2)
                  << (static_cast<double>(usedSlots) / bufferSize) << "\n\n";
    }

    bool add(const Record &record)
    {
        return insert(record);
    }

    int find(const std::string &fio, int number)
    {
        Record recordToFind(fio, number);
        Node *node = findNode(recordToFind);
        return node ? node->value.lineNumber : -1;
    }

    bool remove(const std::string &fio, int number)
    {
        Record recordToDelete(fio, number);
        return deleteElement(recordToDelete);
    }
};

int main()
{
    HashTable<Record> table;
    std::cout << "Initial empty table:\n";
    table.print();

    std::cout << "Adding 6 elements...\n";
    for (int i = 0; i < 6; i++)
    {
        table.add(Record("JohnDoe", "ToyotaCamry", "08:30", 100 + i));
    }
    table.print();

    std::cout << "Adding 4 more elements to trigger resize...\n";
    for (int i = 6; i < 10; i++)
    {
        table.add(Record("JohnDoe", "HondaCivic", "09:15", 100 + i));
    }
    table.print();

    std::cout << "Removing 5 elements to trigger downsizing...\n";
    for (int i = 0; i < 5; i++)
    {
        table.remove("JohnDoe", 100 + i);
    }
    table.print();

    return 0;
}