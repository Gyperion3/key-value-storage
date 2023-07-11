#pragma once
#ifndef STORAGE_H
#define STORAGE_H


// Структура для хранения значения
typedef struct {
    char* key;
    char* value;
} KeyValue;

// Инициализация хранилища
void initialize();

// Запись значения по ключу
void writeValue(const char* key, const char* value);

// Чтение значения по ключу
char* readValue(const char* key);

// Изменение значения по ключу
void updateValue(const char* key, const char* value);

// Очистка хранилища
void clearStorage();

#endif
