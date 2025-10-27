#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
/*
 * Tester, Test your drive speed
 * Copyright (C) 2025 "MayTill"
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of the project "Tester".
 * See LICENSE file for details.
 * If you don't get full file get it at 
 * https://github.com/MayTill/Tester/blob/main/TESTER.cpp
 */
using namespace std;
using namespace std::chrono;

constexpr DWORD BUF_SIZE = 4096 * 256; // 1 MB
constexpr size_t ALIGNMENT = 4096;

atomic<long long> progress_bytes(0);
atomic<long long> total_bytes(0);
atomic<bool> stop_progress(false);

void* AlignedAlloc(size_t size, size_t alignment = ALIGNMENT) {
    return _aligned_malloc(size, alignment);
}

void AlignedFree(void* ptr) {
    _aligned_free(ptr);
}

// --- Wątek monitorujący postęp ---
void ProgressThread(const string& label) {
    using namespace std::chrono_literals;
    while (!stop_progress.load()) {
        long long done = progress_bytes.load();
        long long total = total_bytes.load();
        if (total > 0) {
            double percent = 100.0 * done / total;
            cout << "\r" << label << ": " << fixed << setprecision(1)
                 << percent << "% (" << done / 1048576 << " MB)";
        }
        this_thread::sleep_for(200ms);
    }
    cout << "\r" << label << ": 100.0% (" << total_bytes / 1048576 << " MB)\n";
}

long double WRITE(int MB, char* buffer)
{
    HANDLE hFile = CreateFileW(
        L"test.txt",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Błąd: nie można stworzyć pliku test.txt\n";
        return 0;
    }

    total_bytes = static_cast<long long>(MB) * 1048576;
    progress_bytes = 0;
    stop_progress = false;

    thread progress(ProgressThread, "Zapis");

    auto start = steady_clock::now();
    DWORD bytesWritten = 0;
    long long written = 0;

    while (written < total_bytes) {
        if (!WriteFile(hFile, buffer, BUF_SIZE, &bytesWritten, NULL)) {
            cerr << "\nBłąd zapisu!\n";
            CloseHandle(hFile);
            stop_progress = true;
            progress.join();
            return 0;
        }
        written += bytesWritten;
        progress_bytes = written;
    }

    CloseHandle(hFile);
    auto end = steady_clock::now();

    stop_progress = true;
    progress.join();

    return duration_cast<duration<long double>>(end - start).count();
}

long double READ(int MB, char* buffer)
{
    HANDLE hFile = CreateFileW(
        L"test.txt",
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Błąd: nie można otworzyć pliku test.txt\n";
        return 0;
    }

    total_bytes = static_cast<long long>(MB) * 1048576;
    progress_bytes = 0;
    stop_progress = false;

    thread progress(ProgressThread, "Odczyt");

    auto start = steady_clock::now();
    DWORD bytesRead = 0;
    long long read = 0;

    while (read < total_bytes) {
        if (!ReadFile(hFile, buffer, BUF_SIZE, &bytesRead, NULL)) {
            cerr << "\nBłąd odczytu!\n";
            CloseHandle(hFile);
            stop_progress = true;
            progress.join();
            return 0;
        }
        if (bytesRead == 0) break;
        read += bytesRead;
        progress_bytes = read;
    }

    CloseHandle(hFile);
    auto end = steady_clock::now();

    stop_progress = true;
    progress.join();

    return duration_cast<duration<long double>>(end - start).count();
}

int main()
{
    char* buffer = static_cast<char*>(AlignedAlloc(BUF_SIZE));
    if (!buffer) {
        cerr << "Błąd alokacji bufora!\n";
        return 1;
    }
    memset(buffer, 0, BUF_SIZE);

    while (true) {
        int MB = 0;
        cout << "\n=== TESTER v3.0.1 ===\n";
        cout << "Podaj -1 aby wyjść\n";
        cout << "Ile MB: ";
        cin >> MB;
        if (MB < 0) break;

        cout << "\nTest zapisu...\n";
        long double WT = WRITE(MB, buffer);

        cout << "\nTest odczytu...\n";
        long double RT = READ(MB, buffer);

        cout << "\n\n--- Wyniki ---\n";
        cout << "Zapis: " << fixed << setprecision(2) << (MB / WT) << " MB/s (czas: " << WT << " s)\n";
        cout << "Odczyt: " << fixed << setprecision(2) << (MB / RT) << " MB/s (czas: " << RT << " s)\n";

        DeleteFileW(L"test.txt");

        cout << "\nNaciśnij Enter, aby kontynuować...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
        system("cls");
    }

    AlignedFree(buffer);
    return 0;
}


