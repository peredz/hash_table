#include <iostream>
#include <cmath>

class Record
{
public:
    std::string fio;
    std::string carInfo;
    std::string time;
    int number;

    int getKey()
    {
        int key = 0;
        for (char c : fio)
        {
            key += (int)c;
        }
        return key + number;
    }
};

int NumOfDigits(int number)
{
    int i = 0;
    while (std::pow(10, i) < number)
    {
        i++;
    }
    return i;
}

template <typename T = Record>
int HashFunctionQuadratic(T record, int table_size)
{
    int hash_result = record.getKey();
    hash_result *= hash_result;
    int numOfDigitsInHash = NumOfDigits(hash_result);
    int numOfDigitsInSize = NumOfDigits(table_size);
    hash_result = ((hash_result % (int)pow(10, numOfDigitsInHash - numOfDigitsInSize / 2 + 1)) / (int)pow(10, numOfDigitsInSize / 2)) % table_size;
    return hash_result;
}

template <typename T = Record>
struct HashFunction1
{
    int operator()(T record, int table_size) const
    {
        return HashFunctionQuadratic(record, table_size);
    }
};

struct HashFunction2
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

template <typename T = std::string, class THash1 = HashFunction1<T>, class THash2 = HashFunction2>
class HashTable
{
private:
    class Record
    {
        T value;
        char status;
    };

    using value_type = T;
    static const int default_size = 8;
    constexpr static const double rehash_size = 0.75;
    int buffer_size;
    int size;
    int size_all;
    Record *data;

    void Resize() {}
    void Rehash() {}

public:
    HashTable()
    {
        buffer_size = default_size;
        size = 0;
        size_all = 0;
        data = new Record[default_size];
    }

    value_type Finde(std::string fio, int number) {}
    bool Remove() {}
    bool Add(value_type record)
    {
    }
};