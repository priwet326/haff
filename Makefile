CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = huffman
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) encode_* decode_*

rebuild: clean all

help:
	@echo "Использование:"
	@echo "  make          - собрать программу"
	@echo "  make clean    - удалить исполняемый файл и созданные encode/decode файлы"
	@echo "  make rebuild  - пересобрать программу"
	@echo "  make help     - показать эту справку"
	@echo ""
	@echo "Запуск программы:"
	@echo "  ./$(TARGET) -e <input_file>      # расчет энтропии файла"
	@echo "  ./$(TARGET) <input_file>         # закодировать файл"
	@echo "  ./$(TARGET) -d <encoded_file>    # декодировать файл"
	@echo ""
	@echo "Примеры:"
	@echo "  ./$(TARGET) -e document.txt"
	@echo "  ./$(TARGET) document.txt"
	@echo "  ./$(TARGET) -d encode_document.bin"

.PHONY: all clean rebuild help
