#include <iostream>
#include <cmath>
#include <string>
#include <iomanip>

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

    static const int DEFAULT_SIZE = 8;
    static constexpr double LOAD_FACTOR = 0.75;
    int bufferSize;
    int size;
    int usedSlots;
    Node** table; // Изменено на массив указателей

    bool insert(ValueType value, bool rehash = false)
    {
        Hash1 h1;
        Hash2 h2;
        int initialIndex = h1(value, bufferSize);
        Node* newNode = new Node(std::move(value));

        if (table[initialIndex] == nullptr || table[initialIndex]->status != 1)
        {
            if (table[initialIndex] == nullptr)
            {
                size++;
            }
            else if (table[initialIndex]->status == 0)
            {
                size++;
            }
            
            delete table[initialIndex]; // Освобождаем память, если там что-то было
            table[initialIndex] = newNode;
            
            if (!rehash)
                usedSlots++;
            if (!rehash)
                checkForResize();
            return true;
        }

        int j = 1;
        int probeIndex = initialIndex;
        while (table[probeIndex] != nullptr && table[probeIndex]->status == 1)
        {
            if (table[probeIndex]->value.compareKeys(newNode->value))
            {
                delete newNode; // Не вставили - освобождаем память
                return false;
            }
            probeIndex = h2(initialIndex, j) % bufferSize;
            j++;
        }

        if (table[probeIndex] == nullptr)
        {
            size++;
        }
        else
        {
            delete table[probeIndex]; // Освобождаем память перед заменой
        }
        
        table[probeIndex] = newNode;

        if (!rehash)
            usedSlots++;
        if (!rehash)
            checkForResize();
        return true;
    }

    void checkForResize()
    {
        double actualLoadPercentage = double(usedSlots) / double(bufferSize);

        Node** old_table = table;
        int old_size = bufferSize;

        if (actualLoadPercentage > LOAD_FACTOR)
        {
            resize(true);
        }
        else if (actualLoadPercentage < (1 - LOAD_FACTOR))
        {
            resize(false);
        }
        else
        {
            return;
        }

        // Выделяем новую память и инициализируем nullptr
        table = new Node*[bufferSize];
        for (int i = 0; i < bufferSize; i++)
        {
            table[i] = nullptr;
        }

        rehash(old_table, old_size);
        
        // Освобождаем старую память
        for (int i = 0; i < old_size; i++)
        {
            delete old_table[i];
        }
        delete[] old_table;
    }

    void resize(bool grow)
    {
        if (grow)
        {
            bufferSize = bufferSize * 2;
            return;
        }
        if (bufferSize > DEFAULT_SIZE)
        {
            bufferSize = bufferSize / 2;
        }
    }

    void rehash(Node** old_table, int old_size)
    {
        size = 0;
        usedSlots = 0;
        
        for (int i = 0; i < old_size; i++)
        {
            if (old_table[i] != nullptr && old_table[i]->status == 1)
            {
                insert(old_table[i]->value, true);
            }
        }
    }

    Node* findeNode(ValueType value)
    {
        Hash1 h1;
        Hash2 h2;
        int initialIndex = h1(value, bufferSize);

        if (table[initialIndex] != nullptr && 
            table[initialIndex]->value.compareKeys(value) && 
            table[initialIndex]->status == 1)
        {
            return table[initialIndex];
        }

        int j = 1;
        int probeIndex = initialIndex;
        while (table[probeIndex] != nullptr && table[probeIndex]->status != 0)
        {
            if (table[probeIndex]->value.compareKeys(value) && 
                table[probeIndex]->status == 1)
            {
                return table[probeIndex];
            }
            probeIndex = h2(initialIndex, j) % bufferSize;
            j++;
        }

        return nullptr;
    }

    bool deteleElement(Record recordToDelete)
    {
        Node* nodeToDelete = findeNode(recordToDelete);

        if (nodeToDelete == nullptr || nodeToDelete->status != 1) { return false; }

        nodeToDelete->disable();
        usedSlots--;
        checkForResize();
        return true;
    }

public:
    HashTable() : bufferSize(DEFAULT_SIZE), size(0), usedSlots(0)
    {
        table = new Node*[DEFAULT_SIZE];
        for (int i = 0; i < DEFAULT_SIZE; i++)
        {
            table[i] = nullptr;
        }
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
    }

    bool add(const Record &record)
    {
        return insert(record);
    }

    int find(const std::string& fio, int number) {
        Record recordToFinde(fio, number);
        Node* node = findeNode(recordToFinde);
        return node ? node->value.lineNumber : -1;
    }

    bool remove(const std::string& fio, int number) {
        Record recordToDelete(fio, number);
        return deteleElement(recordToDelete);
    }
};

int main()
{
    HashTable<Record> table;
    table.print();
    std::cout << std::endl;
    for (int i = 0; i < 10; i++)
    {
        if (table.add(Record("JohnSmith", "ToyotaCamry", "08:30", 490 + i))) { std::cout << "Added ToyotaCamry " << 490 + i << ":\n"; }
        else std::cout << "Error occurred while adding\n";
    }
    table.print();

    if (table.add(Record("JohnSmith", "ToyotaCamry", "08:30", 493))) { std::cout << "Added " << 493 << ":\n"; }
    else std::cout << "\n\nError occurred while adding\n";
    table.print();

    for (int i = 5; i < 12; i += 2)
    {
        if (table.remove("JohnSmith", 490 + i)) { std::cout << "\nJohnSmith - " << 490 + i << " was successfully removed\n\r";}
        std::cout << table.find("JohnSmith", 490 + i);
        table.print();
    }

    return 0;
}