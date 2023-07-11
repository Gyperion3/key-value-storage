#include "Header.h"
//#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 1024

// Открытый интерфейс для работы с хранилищем
// Можно реализовать симулятор памяти SSD/MMC в этом файле

// Внутренние переменные для хранения данных
KeyValue* storage = NULL;  // массив структур KeyValue
int storageSize = 0;  // количество элементов в хранилище


// Инициализация хранилища
void initialize() {
    storage = NULL;
    storageSize = 0;
}

// Запись значения по ключу
void writeValue(const char* key, const char* value) {
    // Проверяем, есть ли уже запись с таким ключом
    for (int i = 0; i < storageSize; i++) {
        if (strcmp(storage[i].key, key) == 0) {
            // Записываем новое значение
            free(storage[i].value);
            storage[i].value = strdup(value);
            return;
        }
    }

    // Создаем новую запись
    KeyValue newItem;
    newItem.key = strdup(key);
    newItem.value = strdup(value);

    // Увеличиваем размер хранилища и добавляем новую запись
    storageSize++;
    storage = realloc(storage, storageSize * sizeof(KeyValue));
    storage[storageSize - 1] = newItem;
}

// Чтение значения по ключу
char* readValue(const char* key) {
    for (int i = 0; i < storageSize; i++) {
        if (strcmp(storage[i].key, key) == 0) {
            return storage[i].value;
        }
    }

    return NULL;
}

// Изменение значения по ключу
void updateValue(const char* key, const char* value) {
    for (int i = 0; i < storageSize; i++) {
        if (strcmp(storage[i].key, key) == 0) {
            free(storage[i].value);
            storage[i].value = strdup(value);
            return;
        }
    }

    printf("Error: Key not found\n");
}

// Очистка хранилища
void clearStorage() {
    for (int i = 0; i < storageSize; i++) {
        free(storage[i].key);
        free(storage[i].value);
    }

    free(storage);
    storage = NULL;
    storageSize = 0;
}