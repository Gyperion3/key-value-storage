﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h> // Для функции clock()
#include "../../Storage/Storage/Header.h"

#define PAGE_SIZE 512       // Размер страницы записи
#define BLOCK_SIZE 16384    // Размер блока очистки
#define NUM_PAGES 1000      // Общее количество страниц в памяти
#define CACHE_SIZE 10       // Размер кэша (количество элементов)

// Массив для эмуляции SSD/MMC памяти
char memory[NUM_PAGES][PAGE_SIZE];

// Флаги для отслеживания использования страниц и блоков
bool page_used[NUM_PAGES] = { false };
bool block_used[NUM_PAGES / (BLOCK_SIZE / PAGE_SIZE)] = { false };

// Структура для хранения значения в хеш-таблице
typedef struct KeyValueNode {
    char* key;
    char* value;
    struct KeyValueNode* next; // указатель на следующий элемент списка (для разрешения коллизий)
} KeyValue;

// Хеш-таблица с разделением цепочек
KeyValue* hash_table[NUM_PAGES];

// Кэш для ускорения произвольного доступа на чтение
struct CacheItem {
    char* key;
    char* value;
};
struct CacheItem cache[CACHE_SIZE];
int cache_size = 0;

// Резервная страница
char reserve_page[PAGE_SIZE];

// Хеш-функция
unsigned int hash_function(const char* key) {
    unsigned int hash = 0;
    int c;
    while ((c = *key++)) {
        hash = hash * 31 + c;
    }
    return hash % NUM_PAGES;
}

// Функция записи данных в память
void write_data(int page_num, const char* data) {
    if (page_num >= 0 && page_num < NUM_PAGES) {
        // Проверка блока на использование перед записью
        int block_num = page_num / (BLOCK_SIZE / PAGE_SIZE);
        if (!block_used[block_num]) {
            // Очистка блока перед записью
            memset(&memory[block_num * (BLOCK_SIZE / PAGE_SIZE)], 0, BLOCK_SIZE);
            block_used[block_num] = true;
        }

        // Проверка страницы на использование перед записью
        if (page_used[page_num]) {
            // Если страница уже использовалась, то очищаем ее перед записью новых данных
            memset(&memory[page_num], 0, PAGE_SIZE);
        }
        else {
            // Если страница еще не использовалась, отмечаем ее как использованную
            page_used[page_num] = true;
        }

        // Запись данных в страницу
        strncpy(memory[page_num], data, PAGE_SIZE);
        printf("Запись данных на страницу %d\n", page_num);
    }
    else {
        printf("Ошибка: недопустимый номер страницы\n");
    }
}

// Функция для атомарной записи данных в память
void atomic_write_data(int page_num, const char* data) {
    // Начало транзакции
    // Запоминаем состояние страницы и блока перед записью
    char page_copy[PAGE_SIZE];
    memcpy(page_copy, memory[page_num], PAGE_SIZE);

    int block_num = page_num / (BLOCK_SIZE / PAGE_SIZE);
    char block_copy[BLOCK_SIZE];
    memcpy(block_copy, &memory[block_num * (BLOCK_SIZE / PAGE_SIZE)], BLOCK_SIZE);

    // Записываем данные в копию памяти
    strncpy(page_copy, data, PAGE_SIZE);

    // Конец транзакции
    // Если транзакция прошла успешно, то сохраняем изменения
    // в исходную память, иначе оставляем ее без изменений
    memcpy(memory[page_num], page_copy, PAGE_SIZE);
    memcpy(&memory[block_num * (BLOCK_SIZE / PAGE_SIZE)], block_copy, BLOCK_SIZE);
    printf("Атомарная запись данных на страницу %d\n", page_num);
}

// Функция чтения данных из памяти
const char* read_data(int page_num) {
    if (page_num >= 0 && page_num < NUM_PAGES) {
        // Проверка CRC страницы
        if (!check_page_crc(page_num)) {
            printf("Ошибка: искажение данных на странице %d\n", page_num);

            // Попробуем восстановить данные из резервной страницы
            if (restore_from_reserve_page(page_num)) {
                printf("Данные восстановлены из резервной страницы\n");
            }
            else {
                printf("Не удалось восстановить данные из резервной страницы\n");
         
                return NULL;
            }
        }

        // Поиск значения в хеш-таблице с разделением цепочек
        unsigned int hash = hash_function(memory[page_num]);
        KeyValue* entry = hash_table[hash];
        while (entry != NULL) {
            if (strcmp(entry->key, memory[page_num]) == 0) {
                // Нашли ключ, добавляем его в кэш и возвращаем значение
                add_to_cache(entry->key, entry->value);
                return entry->value;
            }
            entry = entry->next;
        }
        // Ключ не найден
        return NULL;
    }
    else {
        printf("Ошибка: недопустимый номер страницы\n");
        return NULL;
    }
}

// Функция для восстановления данных из резервной страницы
bool restore_from_reserve_page(int page_num) {
    if (page_num >= 0 && page_num < NUM_PAGES) {
        // Проверяем наличие данных на резервной странице
        if (reserve_page[0] == '\0') {
            return false; // Резервная страница пуста, ничего не восстанавливаем
        }

        // Проверяем CRC резервной страницы
        if (!check_page_crc_reserv()) {
            printf("Ошибка: искажение данных на резервной странице\n");
            return false;
        }

        // Копируем данные из резервной страницы на нужную страницу
        memcpy(memory[page_num], reserve_page, PAGE_SIZE);
        return true;
    }
    else {
        printf("Ошибка: недопустимый номер страницы\n");
        return false;
    }
}

// Функция для проверки CRC резервной страницы
bool check_page_crc_reserv() {
    // Вычисляем CRC для данных на резервной странице
    unsigned char crc = 0;
    for (int i = 0; i < PAGE_SIZE; i++) {
        crc ^= reserve_page[i];
    }

    // Проверяем соответствие вычисленного CRC и сохраненного CRC на резервной странице
    return crc == reserve_page[PAGE_SIZE - 1];
}


// Функция для проверки CRC страницы
bool check_page_crc(int page_num) {
    // Проверяем допустимый номер страницы
    if (page_num < 0 || page_num >= NUM_PAGES) {
        printf("Ошибка: недопустимый номер страницы\n");
        return false;
    }

    // Вычисляем CRC для данных на странице
    unsigned char crc = 0;
    for (int i = 0; i < PAGE_SIZE; i++) {
        crc ^= memory[page_num][i];
    }

    // Проверяем соответствие вычисленного CRC и сохраненного CRC на странице
    return crc == memory[page_num][PAGE_SIZE - 1];
}

// Функция для инициализации хеш-таблицы
void initialize_hash_table() {
    for (int i = 0; i < NUM_PAGES; i++) {
        hash_table[i] = NULL;
    }
}

// Функция для добавления элемента в хеш-таблицу
void add_to_hash_table(const char* key, const char* value) {
    unsigned int hash = hash_function(key);
    KeyValue* new_entry = (KeyValue*)malloc(sizeof(KeyValue));
    if (new_entry == NULL) {
        printf("Ошибка выделения памяти\n");
        return;
    }
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    new_entry->next = NULL;

    if (hash_table[hash] == NULL) {
        // Первый элемент в списке
        hash_table[hash] = new_entry;
    }
    else {
        // Коллизия, добавляем элемент в конец списка
        KeyValue* last_entry = hash_table[hash];
        while (last_entry->next != NULL) {
            last_entry = last_entry->next;
        }
        last_entry->next = new_entry;
    }
}

// Функция для поиска значения по ключу в кэше
const char* find_in_cache(const char* key) {
    for (int i = 0; i < cache_size; i++) {
        if (strcmp(cache[i].key, key) == 0) {
            // Нашли ключ в кэше, возвращаем значение
            return cache[i].value;
        }
    }
    // Ключ не найден в кэше
    return NULL;
}

// Функция для добавления элемента в кэш
void add_to_cache(const char* key, const char* value) {
    if (cache_size < CACHE_SIZE) {
        // В кэше еще есть место, добавляем элемент
        cache[cache_size].key = strdup(key);
        cache[cache_size].value = strdup(value);
        cache_size++;
    }
    else {
        // Кэш заполнен, удаляем самый старый элемент и добавляем новый
        free(cache[0].key);
        free(cache[0].value);
        for (int i = 1; i < CACHE_SIZE; i++) {
            cache[i - 1].key = cache[i].key;
            cache[i - 1].value = cache[i].value;
        }
        cache[CACHE_SIZE - 1].key = strdup(key);
        cache[CACHE_SIZE - 1].value = strdup(value);
    }

}


// Функция для поиска значения по ключу в хеш-таблице
const char* find_in_hash_table(const char* key) {
    unsigned int hash = hash_function(key);
    KeyValue* entry = hash_table[hash];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // Нашли ключ, добавляем его в кэш и возвращаем значение
            add_to_cache(entry->key, entry->value);
            return entry->value;
        }
        entry = entry->next;
    }
    // Ключ не найден
    return NULL;
}


//----TESTS----//




//happy flow - проверка выполнения основных операций без ошибок
void run_happy_flow_tests() {
    // Запись данных на страницы
    write_data(0, "Hello, SSD/MMC!");
    write_data(1, "Test data");
    write_data(2, "12345");

    // Чтение данных со страниц
    const char* data1 = read_data(0);
    const char* data2 = read_data(1);
    const char* data3 = read_data(2);

    printf("Happy flow test results:\n");
    printf("Data 1: %s\nData 2: %s\nData 3: %s\n", data1, data2, data3);

    // Проверка очистки данных
    printf("Clearing data on pages...\n");
    memset(&memory[0], 0, PAGE_SIZE);
    memset(&memory[1], 0, PAGE_SIZE);
    memset(&memory[2], 0, PAGE_SIZE);

    // Чтение данных после очистки
    const char* data_after_clear1 = read_data(0);
    const char* data_after_clear2 = read_data(1);
    const char* data_after_clear3 = read_data(2);

    printf("Data after clearing:\n");
    printf("Data 1: %s\nData 2: %s\nData 3: %s\n", data_after_clear1, data_after_clear2, data_after_clear3);
}





// Тест, нсли произошла потеря питания во время записи
void run_power_loss_test() {
    // Атомарная запись данных на страницу
    atomic_write_data(3, "Данные с потерей пиатния");

    // Предполагаем потерю питания
    printf("Симуляция потери питания...\n");
    // Чтение данных со страницы после восстановления питания
    const char* data = read_data(3);

    printf("Результат теста после потери питания:\n");
    printf("Data: %s\n", data);
}



// Тест, когда значение ключа больше страницы
void run_large_key_test() {
    // создание ключа размером 600 символов
    char large_key[600];
    memset(large_key, 'A', sizeof(large_key));
    large_key[sizeof(large_key) - 1] = '\0'; // завершаем строку нулевым символом

    // запись данных на страницу с большим значением ключа
    write_data(1000, large_key);

    // чтение данных со страницы
    const char* data = read_data(1000);

    printf("Large key test result:\n");
    printf("Data: %s\n", data);
}



// Функция для тестирования случая изменения данных извне
void run_external_change_test() {
    // запись данных на страницы
    write_data(0, "Hello, SSD/MMC!");
    write_data(1, "Test data");
    write_data(2, "12345");

    // чтение данных со страниц
    const char* data1 = read_data(0);
    const char* data2 = read_data(1);
    const char* data3 = read_data(2);

    printf("Initial test results:\n");
    printf("Data 1: %s\nData 2: %s\nData 3: %s\n", data1, data2, data3);

    // эмуляция изменения данных другим процессом
    memset(&memory[0], 'X', PAGE_SIZE);
    memset(&memory[1], 'Y', PAGE_SIZE);
    memset(&memory[2], 'Z', PAGE_SIZE);

    //повторное чтение данных со страниц
    const char* updated_data1 = read_data(0);
    const char* updated_data2 = read_data(1);
    const char* updated_data3 = read_data(2);

    printf("Updated test results:\n");
    printf("Data 1: %s\nData 2: %s\nData 3: %s\n", updated_data1, updated_data2, updated_data3);
}




// Функция для тестирования эффективности чтения/поиска после восстановления питания
void run_power_loss_test() {
    // Атомарная запись данных на страницу
    atomic_write_data(3, "Данные с потерей пиатния");

    // Предполагаем потерю питания
    printf("Симуляция потери питания...\n");
    clock_t start_time = clock();
    while ((clock() - start_time) / CLOCKS_PER_SEC < 5) {
        // Эмулируем задержку в 5 секунд
    }

    // после восстановления питания, читаем данные со страницы
    const char* data = read_data(3);

    printf("Результат теста после потери питания:\n");
    printf("Data: %s\n", data);
}

int main() {
    //тестовая запись данных на страницу 5
    atomic_write_data(5, "Hello, SSD/MMC!");

    //чтение данных со страницы 5
    const char* data = read_data(5);
    printf("Прочитанные данные: %s\n", data);

    //пример использования функций для работы с хранилищем
    initialize_hash_table();
    add_to_hash_table("key1", "value1");
    add_to_hash_table("key2", "value2");
    const char* value1 = find_in_hash_table("key1");
    const char* value2 = find_in_hash_table("key2");
    printf("Value1: %s\nValue2: %s\n", value1, value2);

    // использование алгоритма произвольного доступа на чтение
    const char* value3 = find_in_cache("key1");
    if (value3 == NULL) {
        // Значение не найдено в кэше, считываем его из хеш-таблицы и добавляем в кэш
        value3 = find_in_hash_table("key1");
        if (value3 != NULL) {
            add_to_cache("key1", value3);
        }
    }
    printf("Value3: %s\n", value3);

    // Пример работы с хранилищем
    initialize();
    writeValue("key1", "value1");
    writeValue("key2", "value2");
    const char* storedValue1 = readValue("key1");
    const char* storedValue2 = readValue("key2");
    printf("Stored Value1: %s\nStored Value2: %s\n", storedValue1, storedValue2);
    updateValue("key1", "new_value");
    const char* newValue1 = readValue("key1");
    printf("New Value1: %s\n", newValue1);
    clearStorage();

    return 0;
}
