#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <bitset>
#include <memory>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <iomanip>

using namespace std;
namespace fs = filesystem;

// ===================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====================

// Чтение файла в вектор байтов
vector<unsigned char> readFile(const string& filename) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file) {
        throw runtime_error("Не удалось открыть файл: " + filename);
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        throw runtime_error("Не удалось прочитать файл: " + filename);
    }

    return buffer;
}

// Запись вектора байтов в файл
void writeFile(const string& filename, const vector<unsigned char>& data) {
    ofstream file(filename, ios::binary);
    if (!file) {
        throw runtime_error("Не удалось создать файл: " + filename);
    }

    file.write((char*)data.data(), data.size());
}

// Функция для форматирования размера в читаемый вид
string formatSize(size_t bytes) {
    const size_t GB = 1024 * 1024 * 1024;
    const size_t MB = 1024 * 1024;
    const size_t KB = 1024;

    ostringstream oss;

    if (bytes >= GB) {
        size_t gb = bytes / GB;
        size_t remainder = bytes % GB;
        size_t mb = remainder / MB;
        remainder %= MB;
        size_t kb = remainder / KB;
        size_t b = remainder % KB;

        oss << gb << " ГБ ";
        if (mb > 0 || kb > 0 || b > 0) oss << mb << " МБ ";
        if (kb > 0 || b > 0) oss << kb << " КБ ";
        if (b > 0) oss << b << " Б";
    } else if (bytes >= MB) {
        size_t mb = bytes / MB;
        size_t remainder = bytes % MB;
        size_t kb = remainder / KB;
        size_t b = remainder % KB;

        oss << mb << " МБ ";
        if (kb > 0 || b > 0) oss << kb << " КБ ";
        if (b > 0) oss << b << " Б";
    } else if (bytes >= KB) {
        size_t kb = bytes / KB;
        size_t b = bytes % KB;

        oss << kb << " КБ ";
        if (b > 0) oss << b << " Б";
    } else {
        oss << bytes << " Б";
    }

    return oss.str();
}

// Функция для расчета энтропии
double get_entropy(const vector<unsigned char>& data) {
    if (data.empty()) return 0.0;

    size_t freq[256] = {0};
    for (unsigned char b : data) freq[b]++;

    double entropy = 0.0;
    double size = data.size();

    for (size_t count : freq) {
        if (count) {
            double p = count / size;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

// Функция для создания имени закодированного файла
string getEncodedFilename(const string& inputFile) {
    fs::path path(inputFile);
    string stem = path.stem().string();  // Имя без расширения
    string encodedName = "encode_" + stem + ".bin";

    // Если файл с таким именем уже существует, добавляем номер
    int counter = 1;
    string finalName = encodedName;
    while (fs::exists(finalName)) {
        finalName = "encode_" + stem + "_" + to_string(counter) + ".bin";
        counter++;
    }

    return finalName;
}

// Функция для создания имени декодированного файла
string getDecodedFilename(const string& encodedFile) {
    fs::path path(encodedFile);
    string stem = path.stem().string();  // Имя без расширения

    // Убираем префикс "encode_" если он есть
    if (stem.find("encode_") == 0) {
        stem = stem.substr(7);  // Убираем "encode_"
    }

    string decodedName = "decode_" + stem;

    // Если файл с таким именем уже существует, добавляем номер
    int counter = 1;
    string finalName = decodedName;
    while (fs::exists(finalName)) {
        finalName = "decode_" + stem + "_" + to_string(counter);
        counter++;
    }

    return finalName;
}

// Функция для вывода только энтропии
void printEntropyOnly(const string& filename) {
    try {
        auto data = readFile(filename);
        double entropy = get_entropy(data);
        double theoreticalMinBytes = data.size() * entropy / 8;
        size_t theoreticalMin = static_cast<size_t>(ceil(theoreticalMinBytes));

        cout << "Файл: " << filename << endl;
        cout << "Размер: " << formatSize(data.size()) << " (" << data.size() << " байт)" << endl;
        cout << fixed << setprecision(3);
        cout << "Энтропия: " << entropy << " бит/символ" << endl;
        cout << "Минимальный теоретический размер: " << formatSize(theoreticalMin)
             << " (" << theoreticalMin << " байт)" << endl;
        cout << defaultfloat;
    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
    }
}

// ===================== КЛАСС УЗЛА ДЕРЕВА =====================

struct HuffmanNode {
    unsigned char byte;
    int freq;
    shared_ptr<HuffmanNode> left;
    shared_ptr<HuffmanNode> right;

    HuffmanNode(unsigned char b, int f)
        : byte(b), freq(f), left(nullptr), right(nullptr) {}

    HuffmanNode(int f, shared_ptr<HuffmanNode> l, shared_ptr<HuffmanNode> r)
        : byte(0), freq(f), left(l), right(r) {}

    bool isLeaf() const {
        return !left && !right;
    }
};

// Компаратор для priority_queue
struct CompareNode {
    bool operator()(const shared_ptr<HuffmanNode>& a,
                   const shared_ptr<HuffmanNode>& b) {
        return a->freq > b->freq;
    }
};

// ===================== КЛАСС HUFFMAN =====================

class HuffmanEncoder {
private:
    shared_ptr<HuffmanNode> root;
    map<unsigned char, string> codes;
    map<unsigned char, int> freq;

    // Подсчет частот байтов
    void countFrequencies(const vector<unsigned char>& data) {
        freq.clear();
        for (unsigned char byte : data) {
            freq[byte]++;
        }
    }

    // Построение дерева Хаффмена
    void buildTree() {
        if (freq.empty()) return;

        priority_queue<shared_ptr<HuffmanNode>,
                      vector<shared_ptr<HuffmanNode>>,
                      CompareNode> pq;

        // Создаем листья для каждого байта
        for (const auto& pair : freq) {
            pq.push(make_shared<HuffmanNode>(pair.first, pair.second));
        }

        // Если только один уникальный байт
        if (pq.size() == 1) {
            auto node = pq.top();
            auto parent = make_shared<HuffmanNode>(node->freq, node, nullptr);
            pq.pop();
            pq.push(parent);
        }

        // Строим дерево
        while (pq.size() > 1) {
            auto left = pq.top(); pq.pop();
            auto right = pq.top(); pq.pop();

            auto parent = make_shared<HuffmanNode>(
                left->freq + right->freq, left, right);

            pq.push(parent);
        }

        root = pq.top();
    }

    // Рекурсивная генерация кодов
    void generateCodes(shared_ptr<HuffmanNode> node, const string& code) {
        if (!node) return;

        if (node->isLeaf()) {
            codes[node->byte] = code.empty() ? "0" : code;
            return;
        }

        generateCodes(node->left, code + "0");
        generateCodes(node->right, code + "1");
    }

    // Сериализация дерева (запись в бинарный формат)
    void serializeTree(shared_ptr<HuffmanNode> node, vector<bool>& bits) {
        if (!node) return;

        if (node->isLeaf()) {
            bits.push_back(true); // Маркер листа
            // Записываем байт как 8 бит
            for (int i = 7; i >= 0; i--) {
                bits.push_back((node->byte >> i) & 1);
            }
        } else {
            bits.push_back(false); // Маркер внутреннего узла
            serializeTree(node->left, bits);
            serializeTree(node->right, bits);
        }
    }

    // Десериализация дерева (чтение из бинарного формата)
    shared_ptr<HuffmanNode> deserializeTree(const vector<bool>& bits, size_t& pos) {
        if (pos >= bits.size()) return nullptr;

        bool isLeaf = bits[pos++];

        if (isLeaf) {
            unsigned char byte = 0;
            for (int i = 7; i >= 0; i--) {
                if (pos >= bits.size()) return nullptr;
                byte |= (bits[pos++] << i);
            }
            return make_shared<HuffmanNode>(byte, 0);
        } else {
            auto left = deserializeTree(bits, pos);
            auto right = deserializeTree(bits, pos);
            return make_shared<HuffmanNode>(0, left, right);
        }
    }

    // Преобразование вектора bool в байты
    vector<unsigned char> bitsToBytes(const vector<bool>& bits) {
        vector<unsigned char> bytes;
        unsigned char byte = 0;
        int bitCount = 0;

        for (bool bit : bits) {
            byte = (byte << 1) | bit;
            bitCount++;

            if (bitCount == 8) {
                bytes.push_back(byte);
                byte = 0;
                bitCount = 0;
            }
        }

        // Если остались неполные байты, дополняем нулями
        if (bitCount > 0) {
            byte <<= (8 - bitCount);
            bytes.push_back(byte);
        }

        return bytes;
    }

    // Преобразование байтов в вектор bool
    vector<bool> bytesToBits(const vector<unsigned char>& bytes, size_t totalBits) {
        vector<bool> bits;
        bits.reserve(totalBits);

        for (unsigned char byte : bytes) {
            for (int i = 7; i >= 0; i--) {
                if (bits.size() >= totalBits) break;
                bits.push_back((byte >> i) & 1);
            }
        }

        return bits;
    }

public:
    // Кодирование данных
    vector<unsigned char> encode(const vector<unsigned char>& data) {
        if (data.empty()) return {};

        // 1. Подсчет частот
        countFrequencies(data);

        // 2. Построение дерева
        buildTree();

        // 3. Генерация кодов
        codes.clear();
        generateCodes(root, "");

        // 4. Кодирование данных
        vector<bool> encodedBits;

        // Сначала сериализуем дерево
        vector<bool> treeBits;
        serializeTree(root, treeBits);

        // Добавляем размер дерева в битах (32 бита)
        size_t treeSize = treeBits.size();
        for (int i = 31; i >= 0; i--) {
            encodedBits.push_back((treeSize >> i) & 1);
        }

        // Добавляем само дерево
        encodedBits.insert(encodedBits.end(), treeBits.begin(), treeBits.end());

        // Добавляем размер данных в битах (32 бита)
        size_t dataSize = 0;
        for (unsigned char byte : data) {
            dataSize += codes[byte].length();
        }

        for (int i = 31; i >= 0; i--) {
            encodedBits.push_back((dataSize >> i) & 1);
        }

        // Добавляем закодированные данные
        for (unsigned char byte : data) {
            const string& code = codes[byte];
            for (char c : code) {
                encodedBits.push_back(c == '1');
            }
        }

        // Преобразуем биты в байты
        return bitsToBytes(encodedBits);
    }

    // Декодирование данных
    vector<unsigned char> decode(const vector<unsigned char>& encodedData) {
        if (encodedData.empty()) return {};

        // Преобразуем байты в биты
        vector<bool> bits = bytesToBits(encodedData, encodedData.size() * 8);
        size_t pos = 0;

        // Читаем размер дерева
        size_t treeSize = 0;
        for (size_t i = 0; i < 32; i++) {
            if (pos >= bits.size()) return {};
            treeSize = (treeSize << 1) | bits[pos++];
        }

        // Читаем дерево
        size_t treeEndPos = pos + treeSize;
        if (treeEndPos > bits.size()) return {};

        vector<bool> treeBits(bits.begin() + pos, bits.begin() + treeEndPos);
        pos = 0;
        root = deserializeTree(treeBits, pos);
        if (!root) return {};

        // Читаем размер данных
        size_t dataSize = 0;
        for (size_t i = 0; i < 32; i++) {
            if (treeEndPos + i >= bits.size()) return {};
            dataSize = (dataSize << 1) | bits[treeEndPos + i];
        }

        pos = treeEndPos + 32;

        // Декодируем данные
        vector<unsigned char> decoded;
        auto current = root;

        for (size_t i = 0; i < dataSize; i++) {
            if (pos >= bits.size()) break;

            if (bits[pos++]) {
                current = current->right;
            } else {
                current = current->left;
            }

            if (!current) break;

            if (current->isLeaf()) {
                decoded.push_back(current->byte);
                current = root;
            }
        }

        return decoded;
    }
};

// ===================== ОСНОВНАЯ ПРОГРАММА =====================

void printUsage(const char* progName) {
    cout << "Использование:\n";
    cout << "  Расчет энтропии: " << progName << " -e <input_file>\n";
    cout << "  Кодирование: " << progName << " <input_file>\n";
    cout << "  Декодирование: " << progName << " -d <encoded_file>\n";
    cout << "\nПримеры:\n";
    cout << "  " << progName << " -e document.txt           # расчет энтропии\n";
    cout << "  " << progName << " document.txt              # кодирование в encode_document.bin\n";
    cout << "  " << progName << " -d encode_document.bin    # декодирование в decode_document\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        string inputFile, outputFile;
        bool decodeMode = false;
        bool entropyMode = false;

        // Парсинг аргументов
        if (argc == 2) {
            // Режим кодирования: ./a.out file.txt
            inputFile = argv[1];
            outputFile = getEncodedFilename(inputFile);
            decodeMode = false;
            entropyMode = false;
        } else if (argc == 3) {
            string flag = argv[1];
            if (flag == "-d") {
                // Режим декодирования: ./a.out -d encoded.bin
                inputFile = argv[2];
                outputFile = getDecodedFilename(inputFile);
                decodeMode = true;
                entropyMode = false;
            } else if (flag == "-e") {
                // Режим расчета энтропии: ./a.out -e file.txt
                inputFile = argv[2];
                entropyMode = true;
            } else {
                printUsage(argv[0]);
                return 1;
            }
        } else {
            printUsage(argv[0]);
            return 1;
        }

        if (entropyMode) {
            // РЕЖИМ РАСЧЕТА ЭНТРОПИИ
            printEntropyOnly(inputFile);
            return 0;
        }

        HuffmanEncoder encoder;

        if (!decodeMode) {
            // КОДИРОВАНИЕ
            cout << "Кодирование файла: " << inputFile << endl;
            cout << "Результат будет сохранен в: " << outputFile << endl;

            // Чтение исходного файла
            auto inputData = readFile(inputFile);
            size_t inputSize = inputData.size();
            cout << "Размер исходного файла: " << formatSize(inputSize)
                 << " (" << inputSize << " байт)" << endl;

            // Расчет энтропии исходного файла
            double inputEntropy = get_entropy(inputData);
            double theoreticalMinBytes = inputSize * inputEntropy / 8;
            size_t theoreticalMin = static_cast<size_t>(ceil(theoreticalMinBytes));

            cout << fixed << setprecision(3);
            cout << "Энтропия исходного файла: " << inputEntropy << " бит/символ" << endl;
            cout << "Теоретический предел сжатия: " << formatSize(theoreticalMin)
                 << " (" << theoreticalMin << " байт)" << endl;
            cout << defaultfloat;

            // Кодирование
            auto encodedData = encoder.encode(inputData);
            size_t encodedSize = encodedData.size();
            cout << "Размер после сжатия: " << formatSize(encodedSize)
                 << " (" << encodedSize << " байт)" << endl;

            // Расчет энтропии сжатого файла
            double encodedEntropy = get_entropy(encodedData);
            cout << fixed << setprecision(3);
            cout << "Энтропия сжатого файла: " << encodedEntropy << " бит/символ" << endl;
            cout << defaultfloat;

            // Запись результата
            writeFile(outputFile, encodedData);
            cout << "Сжатый файл успешно сохранен." << endl;

            if (!inputData.empty()) {
                double ratio = (1.0 - (double)encodedSize / inputSize) * 100;
                cout << fixed << setprecision(2);
                cout << "Степень сжатия: " << ratio << "%" << endl;

                // Эффективность сжатия относительно энтропии
                if (theoreticalMin > 0) {
                    double efficiency = (theoreticalMinBytes / encodedSize) * 100;
                    cout << "Эффективность относительно энтропии: " << efficiency << "%" << endl;
                }
                cout << defaultfloat;
            }

        } else {
            // ДЕКОДИРОВАНИЕ
            cout << "Декодирование файла: " << inputFile << endl;
            cout << "Результат будет сохранен в: " << outputFile << endl;

            // Чтение закодированного файла
            auto encodedData = readFile(inputFile);
            size_t encodedSize = encodedData.size();
            cout << "Размер закодированного файла: " << formatSize(encodedSize)
                 << " (" << encodedSize << " байт)" << endl;

            // Расчет энтропии закодированного файла
            double encodedEntropy = get_entropy(encodedData);
            cout << fixed << setprecision(3);
            cout << "Энтропия закодированного файла: " << encodedEntropy << " бит/символ" << endl;
            cout << defaultfloat;

            // Декодирование
            auto decodedData = encoder.decode(encodedData);
            size_t decodedSize = decodedData.size();
            cout << "Размер после декодирования: " << formatSize(decodedSize)
                 << " (" << decodedSize << " байт)" << endl;

            // Расчет энтропии декодированного файла
            double decodedEntropy = get_entropy(decodedData);
            cout << fixed << setprecision(3);
            cout << "Энтропия восстановленного файла: " << decodedEntropy << " бит/символ" << endl;
            cout << defaultfloat;

            // Запись результата
            writeFile(outputFile, decodedData);
            cout << "Восстановленный файл успешно сохранен." << endl;

            // Проверка целостности
            if (!decodedData.empty()) {
                cout << "Восстановление завершено успешно." << endl;
            }
        }

    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }

    return 0;
}
