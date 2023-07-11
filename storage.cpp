#include "Header.h"
//#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 1024

// �������� ��������� ��� ������ � ����������
// ����� ����������� ��������� ������ SSD/MMC � ���� �����

// ���������� ���������� ��� �������� ������
KeyValue* storage = NULL;  // ������ �������� KeyValue
int storageSize = 0;  // ���������� ��������� � ���������


// ������������� ���������
void initialize() {
    storage = NULL;
    storageSize = 0;
}

// ������ �������� �� �����
void writeValue(const char* key, const char* value) {
    // ���������, ���� �� ��� ������ � ����� ������
    for (int i = 0; i < storageSize; i++) {
        if (strcmp(storage[i].key, key) == 0) {
            // ���������� ����� ��������
            free(storage[i].value);
            storage[i].value = strdup(value);
            return;
        }
    }

    // ������� ����� ������
    KeyValue newItem;
    newItem.key = strdup(key);
    newItem.value = strdup(value);

    // ����������� ������ ��������� � ��������� ����� ������
    storageSize++;
    storage = realloc(storage, storageSize * sizeof(KeyValue));
    storage[storageSize - 1] = newItem;
}

// ������ �������� �� �����
char* readValue(const char* key) {
    for (int i = 0; i < storageSize; i++) {
        if (strcmp(storage[i].key, key) == 0) {
            return storage[i].value;
        }
    }

    return NULL;
}

// ��������� �������� �� �����
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

// ������� ���������
void clearStorage() {
    for (int i = 0; i < storageSize; i++) {
        free(storage[i].key);
        free(storage[i].value);
    }

    free(storage);
    storage = NULL;
    storageSize = 0;
}