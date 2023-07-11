#pragma once
#ifndef STORAGE_H
#define STORAGE_H


// ��������� ��� �������� ��������
typedef struct {
    char* key;
    char* value;
} KeyValue;

// ������������� ���������
void initialize();

// ������ �������� �� �����
void writeValue(const char* key, const char* value);

// ������ �������� �� �����
char* readValue(const char* key);

// ��������� �������� �� �����
void updateValue(const char* key, const char* value);

// ������� ���������
void clearStorage();

#endif
