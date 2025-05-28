#include <iostream>
#include <cmath>
#include <string>
#include <iomanip>
#include <fstream>

class Record
{
public:
    std::string fullName;
    std::string carModel;
    std::string time;
    int lineNumber;
    int clientId;

    Record() : fullName(""), carModel(""), time(""), clientId(0), lineNumber(-1) {}

    Record(std::string name, int id)
        : fullName(std::move(name)), carModel(""), time(""), clientId(id), lineNumber(-1) {}

    Record(std::string name, std::string car, std::string timeStr, int id, int lineNum = -1)
        : fullName(std::move(name)), carModel(std::move(car)),
          time(std::move(timeStr)), clientId(id), lineNumber(lineNum) {}

    int calculateHashKey() const
    {
        int key = 0;
        for (char c : fullName)
        {
            key += static_cast<int>(c);
        }
        return key + clientId;
    }

    bool isSameRecord(const Record &other) const
    {
        return (fullName == other.fullName && clientId == other.clientId);
    }

    void printKey() const
    {
        std::cout << fullName << " " << clientId;
    }

    void printDetails() const
    {
        std::cout << " " << carModel;
    }

    void print() const
    {
        printKey();
        printDetails();
    }
};

template <typename T = Record>
int midSquareHash(const T &record, int tableSize)
{
    int key = record.calculateHashKey();
    int squared = key * key;
    std::string squaredStr = std::to_string(squared);
    int length = squaredStr.size();
    int middle = length / 2;
    int digitsNeeded = std::to_string(tableSize).size();

    std::string middleDigits = squaredStr.substr(std::max(0, middle - digitsNeeded / 2), digitsNeeded);
    if (middleDigits.empty())
        return 0;

    return std::stoi(middleDigits) % tableSize;
}

template <typename T = Record>
struct PrimaryHashFunction
{
    int operator()(const T &record, int tableSize) const
    {
        return midSquareHash(record, tableSize);
    }
};

struct QuadraticProbingFunction
{
private:
    const int linearCoefficient = 1;
    const int quadraticCoefficient = 2;

public:
    int operator()(int hash, int attempt) const
    {
        return hash + linearCoefficient * attempt + quadraticCoefficient * attempt * attempt;
    }
};

template <typename T = Record,
          class PrimaryHash = PrimaryHashFunction<T>,
          class CollisionResolver = QuadraticProbingFunction>
class HashTable
{
public:
    struct TableEntry
    {
        T data;
        char state; // 0 = empty, 1 = occupied, 2 = deleted

        TableEntry() : state(0) {}
        TableEntry(T value) : data(std::move(value)), state(1) {}

        void markAsDeleted()
        {
            data.lineNumber = -1;
            state = 2;
        }
    };

private:
    static const int INITIAL_CAPACITY;
    static constexpr double MAX_LOAD_FACTOR = 0.75;
    static constexpr double MIN_LOAD_FACTOR = 0.25;

    int currentCapacity;
    int totalEntries;
    int occupiedSlots;
    TableEntry *hashTable;

    bool insertRecord(T record, bool isRehashing = false)
    {
        PrimaryHash primaryHash;
        CollisionResolver resolveCollision;

        int initialIndex = primaryHash(record, currentCapacity);
        TableEntry newEntry(std::move(record));

        if (hashTable[initialIndex].state != 1)
        {
            if (hashTable[initialIndex].state == 0)
            {
                totalEntries++;
            }
            if (!isRehashing)
                occupiedSlots++;
            hashTable[initialIndex] = newEntry;
            if (!isRehashing)
                checkLoadFactor();
            return true;
        }

        int attempt = 1;
        int currentIndex = initialIndex;
        while (hashTable[currentIndex].state == 1)
        {
            if (hashTable[currentIndex].data.isSameRecord(newEntry.data))
            {
                return false;
            }
            currentIndex = resolveCollision(initialIndex, attempt) % currentCapacity;
            attempt++;
        }

        hashTable[currentIndex] = newEntry;

        if (hashTable[currentIndex].state == 0)
        {
            totalEntries++;
        }
        if (!isRehashing)
            occupiedSlots++;
        if (!isRehashing)
            checkLoadFactor();
        return true;
    }

    void checkLoadFactor()
    {
        double currentLoad = double(occupiedSlots) / double(currentCapacity);

        TableEntry *oldTable = hashTable;
        int oldCapacity = currentCapacity;

        if (currentLoad > MAX_LOAD_FACTOR)
        {
            resizeTable(true);
        }
        else if (currentLoad < MIN_LOAD_FACTOR && currentCapacity > INITIAL_CAPACITY)
        {
            resizeTable(false);
        }
        else
        {
            return;
        }

        hashTable = new TableEntry[currentCapacity];
        rehashTable(oldTable, oldCapacity);
        delete[] oldTable;
    }

    void resizeTable(bool shouldExpand)
    {
        if (shouldExpand)
        {
            currentCapacity *= 2;
        }
        else
        {
            currentCapacity = std::max(INITIAL_CAPACITY, currentCapacity / 2);
        }
    }

    void rehashTable(TableEntry *oldTable, int oldCapacity)
    {
        for (int i = 0; i < oldCapacity; i++)
        {
            if (oldTable[i].state == 1)
            {
                insertRecord(oldTable[i].data, true);
            }
        }
        totalEntries = occupiedSlots;
    }

    TableEntry findEntry(const T &record, bool shouldMarkDeleted = false)
    {
        PrimaryHash primaryHash;
        CollisionResolver resolveCollision;

        int initialIndex = primaryHash(record, currentCapacity);

        if (hashTable[initialIndex].data.isSameRecord(record) &&
            hashTable[initialIndex].state == 1)
        {
            if (shouldMarkDeleted)
                hashTable[initialIndex].markAsDeleted();
            else std::cout << "It took 1 step to finde the Record. Line number: " << hashTable[initialIndex].data.lineNumber << std::endl;
            return hashTable[initialIndex];
        }

        int attempt = 1;
        int currentIndex = initialIndex;
        while (hashTable[currentIndex].state != 0)
        {
            if (hashTable[currentIndex].data.isSameRecord(record) &&
                hashTable[currentIndex].state == 1)
            {
                if (shouldMarkDeleted)
                    hashTable[currentIndex].markAsDeleted();
                else std::cout << "It took " << attempt + 1 << " steps to finde the Record. Line number: " << hashTable[initialIndex].data.lineNumber << std::endl;
                return hashTable[currentIndex];
            }
            currentIndex = resolveCollision(initialIndex, attempt) % currentCapacity;
            attempt++;
        }

        return TableEntry();
    }

    bool removeRecord(const T &record)
    {
        TableEntry foundEntry = findEntry(record, true);

        if (foundEntry.state == 0)
        {
            return false;
        }

        occupiedSlots--;
        checkLoadFactor();
        return true;
    }

    void loadDataFromFile(const std::string &fileName)
    {
        // Проверка возможности открытия файла
        std::ifstream input(fileName);
        if (!input.is_open())
        {
            std::cerr << "[Error] Failed to open file: " << fileName << std::endl;
            return;
        }

        // Ввод количества записей с проверкой
        int count;
        std::cout << "[Enter count of lines to read] ";
        if (!(std::cin >> count) || count <= 0)
        {
            std::cerr << "[Error] Invalid count value" << std::endl;
            return;
        }

        // Проверка, что файл не пустой
        if (input.peek() == std::ifstream::traits_type::eof())
        {
            std::cerr << "[Error] File is empty" << std::endl;
            return;
        }

        // Ввод ключей для поиска
        int number;
        std::string fio;

        // Чтение данных из файла
        int successfullyRead = 0;
        Record newRecord;
        for (int i = 0; i < count; i++)
        {
            if (!(input >> newRecord.fullName >> newRecord.carModel >> newRecord.time >> newRecord.clientId))
            {
                if (input.eof())
                {
                    std::cerr << "[Warning] Reached end of file after reading " << successfullyRead
                              << " records (requested " << count << ")" << std::endl;
                }
                else
                {
                    std::cerr << "[Error] Failed to read record #" << i + 1
                              << " (wrong data format)" << std::endl;
                }
                break;
            }

            newRecord.lineNumber = i + 1;
            if (!insertRecord(newRecord))
            {
                std::cerr << "[Warning] Failed to insert record #" << i + 1
                          << " (possible duplicate)" << std::endl;
                continue;
            }
            successfullyRead++;
        }

        std::cout << "Successfully loaded " << successfullyRead << " records from file: " << fileName << std::endl;
    }

    void saveDataToFile(const std::string &fileName)
    {
        std::ofstream output(fileName, std::ios::out | std::ios::trunc);
        if (!output)
        {
            std::cerr << "[Error] Failed to open file: " << fileName << std::endl;
            return;
        };
        formatTablePrint(output);
        output.close();
    }

    void formatTablePrint(std::ostream& stream = std::cout) const
    {
        stream << std::left;
        stream << std::setw(6) << "Index"
               << std::setw(25) << "Key (Name + ID)"
               << std::setw(25) << "Car Model"
               << std::setw(12) << "Status" << "\n";

        stream << std::string(68, '-') << "\n";

        for (int i = 0; i < currentCapacity; ++i)
        {
            stream << std::setw(6) << i;

            if (hashTable[i].state == 0)
            {
                stream << std::setw(25) << "no data"
                       << std::setw(25) << "-"
                       << std::setw(12) << "empty";
            }
            else if (hashTable[i].state == 1)
            {
                std::string keyStr = hashTable[i].data.fullName + " " +
                                     std::to_string(hashTable[i].data.clientId);
                stream << std::setw(25) << keyStr
                       << std::setw(25) << hashTable[i].data.carModel
                       << std::setw(12) << "occupied";
            }
            else // state == 2
            {
                std::string keyStr = hashTable[i].data.fullName + " " +
                                     std::to_string(hashTable[i].data.clientId);
                stream << std::setw(25) << keyStr
                       << std::setw(25) << "-"
                       << std::setw(12) << "deleted";
            }
            stream << "\n";
        }
        stream << "Capacity: " << currentCapacity
               << ", Occupied: " << occupiedSlots
               << ", Load factor: " << std::fixed << std::setprecision(2)
               << (static_cast<double>(occupiedSlots) / currentCapacity) << "\n\n";
    }

public:
    HashTable() : currentCapacity(INITIAL_CAPACITY),
                  totalEntries(0),
                  occupiedSlots(0)
    {
        hashTable = new TableEntry[INITIAL_CAPACITY];
    }

    ~HashTable()
    {
        delete[] hashTable;
    }

    void print() const
    {
        formatTablePrint();
    }

    bool add(const Record &record)
    {
        return insertRecord(record);
    }

    int find(const std::string &name, int id)
    {
        Record recordToFind(name, id);
        return findEntry(recordToFind).data.lineNumber;
    }

    bool remove(const std::string &name, int id)
    {
        Record recordToRemove(name, id);
        return removeRecord(recordToRemove);
    }

    void loadData(const std::string &fileName)
    {
        loadDataFromFile(fileName);
    }

    void saveTable(const std::string &fileName)
    {
        saveDataToFile(fileName);
    }
};

template <typename T, class PrimaryHash, class CollisionResolver>
const int HashTable<T, PrimaryHash, CollisionResolver>::INITIAL_CAPACITY = 8;

void tests()
{
    HashTable<Record> parkingSystem;
    std::cout << "Initial empty parking system:\n";
    parkingSystem.print();

    // Тест 1: Добавление первых 10 записей
    std::cout << "\n=== Test 1: Adding initial 10 records ===\n";
    for (int i = 0; i < 10; i++)
    {
        std::string carModel = "ToyotaCamry";
        std::string time = "08:" + std::to_string(30 + i);
        int clientId = 490 + i;

        if (parkingSystem.add(Record("JohnSmith", carModel, time, clientId)))
        {
            std::cout << "Added: JohnSmith (ID: " << clientId << ") at " << time << "\n";
        }
        else
        {
            std::cout << "[Error] Failed to add client " << clientId << "\n";
        }
    }
    std::cout << "\nParking system after adding 10 records:\n";
    parkingSystem.print();

    // Тест 2: Попытка добавить дубликат
    std::cout << "\n=== Test 2: Trying to add duplicate record ===\n";
    int duplicateId = 493;
    if (parkingSystem.add(Record("JohnSmith", "ToyotaCamry", "08:33", duplicateId)))
    {
        std::cout << "Unexpectedly added duplicate ID " << duplicateId << "\n";
    }
    else
    {
        std::cout << "Correctly rejected duplicate ID " << duplicateId << "\n";
    }
    parkingSystem.print();

    // Тест 3: Удаление нескольких записей (нечетные ID в диапазоне 495-501)
    std::cout << "\n=== Test 3: Removing selected records ===\n";
    for (int i = 5; i < 12; i += 2)
    {
        int clientId = 490 + i;
        if (parkingSystem.remove("JohnSmith", clientId))
        {
            std::cout << "Successfully removed client " << clientId << "\n";
        }
        else
        {
            std::cout << "[Error] Failed to remove client " << clientId << "\n";
        }
        parkingSystem.print();
    }

    // Тест 4: Удаление первых 5 записей
    std::cout << "\n=== Test 4: Removing first 5 records ===\n";
    for (int i = 0; i < 5; i++)
    {
        int clientId = 490 + i;
        if (parkingSystem.remove("JohnSmith", clientId))
        {
            std::cout << "Successfully removed client " << clientId << "\n";
        }
        else
        {
            std::cout << "[Error] Failed to remove client " << clientId << "\n";
        }
        parkingSystem.print();
    }

    // Тест 5: Массовое добавление 100 записей
    std::cout << "\n=== Test 5: Adding 100 new records ===\n";
    for (int i = 0; i < 100; i++)
    {
        int clientId = 490 + i;
        std::string time = "09:" + std::to_string(i % 60);

        if (parkingSystem.add(Record("JohnSmith", "ToyotaCamry", time, clientId)))
        {
            if (i % 25 == 0)
                std::cout << "Added client " << clientId << "...\n";
        }
        else
        {
            std::cout << "[Error] Failed to add client " << clientId << "\n";
        }
    }
    std::cout << "Finished adding 100 records\n";
    parkingSystem.print();

    // Тест 6: Удаление диапазона записей (ID 530-545)
    std::cout << "\n=== Test 6: Removing range of records (IDs 530-545) ===\n";
    for (int i = 40; i < 55; i++)
    {
        int clientId = 490 + i;
        if (parkingSystem.remove("JohnSmith", clientId))
        {
            std::cout << "Removed client " << clientId << "\n";
        }
        else
        {
            std::cout << "[Error] Client " << clientId << " not found\n";
        }
    }
    parkingSystem.print();

    // Тест 7: Поиск конкретной записи
    std::cout << "\n=== Test 7: Searching for specific record ===\n";
    int searchId = 541;
    int lineNumber = parkingSystem.find("JohnSmith", searchId);
    if (lineNumber != -1)
    {
        std::cout << "Found client " << searchId << " at line " << lineNumber << "\n";
    }
    else
    {
        std::cout << "Client " << searchId << " not found\n";
    }
}

void fileTests()
{
    HashTable<Record> parkingSystem;
    std::cout << "Downloading data from file:\n";
    
    std::string inputFileName = "input.txt";
    parkingSystem.loadData(inputFileName);

    std::string name = "AnthonyHarris";
    int searchId = 888;
    std::cout << "Finding record: \""  << name << " " << searchId << "\"\n";
    int lineNumber = parkingSystem.find(name, searchId);

    if (lineNumber != -1)
    {
        std::cout << "Found client " <<name << " id: " << searchId << " at line " << lineNumber << "\n";
    }
    else
    {
        std::cout << "Client " <<name << " id: " << searchId  << " not found\n";
    }

    std::string outputFileName = "output.txt";
    parkingSystem.saveTable(outputFileName);
}

int main()
{
    // tests();
    fileTests();
}