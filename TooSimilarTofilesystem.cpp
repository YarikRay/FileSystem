#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <stack>
#include <windows.h>
#include <deque>
#include <algorithm> // для std::find_last_of

using namespace std;

#pragma pack(push,1)

struct file_info {
    bool directory;
    wstring full_path;
    wstring name;
    wstring d_name;
    int size;
    wstring extension;
};

template<typename T>
class Iterator {
private:
    T* current_position;
    T* end;
public:
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    Iterator(T* start_pos, T* end_pos) : current_position(start_pos), end(end_pos) {}

    Iterator& operator++() {
        if (current_position != end) {
            ++current_position;
        }
        return *this;
    }

    reference operator*() {
        return *current_position;
    }

    bool operator!=(const Iterator&) const {
        return this->current_position != end;
    }
};

template<typename T>
class Directory_It {
private:
    WIN32_FIND_DATAW ffd; // Явно указываем wide-версию
    file_info file;

public:
    vector<T> info;

    Directory_It(wstring path_ent) { enumeration_method(path_ent); }

    int calc_size(wstring path_ent) {
        int size = 0;
        deque<wstring> dirs_to_process = { path_ent };

        while (!dirs_to_process.empty()) {
            wstring current_path = dirs_to_process.front();
            dirs_to_process.pop_front();

            wstring search_path = current_path + L"\\*";
            HANDLE hFind = FindFirstFileW(search_path.c_str(), &ffd);

            if (hFind == INVALID_HANDLE_VALUE) {
                DWORD err = GetLastError();
                if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
                    wcerr << L"Ошибка доступа: " << search_path << endl;
                }
                continue;
            }

            do {
                if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                    continue;
                }

                wstring full_path = current_path + L"\\" + ffd.cFileName;

                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    dirs_to_process.push_back(full_path);
                }
                else {
                    HANDLE hFile = CreateFileW(
                        full_path.c_str(),
                        GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        DWORD filesize = GetFileSize(hFile, NULL);
                        size += filesize;
                    }

                    // ifstream in(full_path, ios::binary | ios::ate);
                    // if (in.is_open()) {
                    //     size += in.tellg();
                    //     in.close();
                    // }
                }
            } while (FindNextFileW(hFind, &ffd));

            FindClose(hFind);
        }
        return size;
    }

    void enumeration_method(wstring path_ent) {
        deque<wstring> dirs_to_process = { path_ent };

        while (!dirs_to_process.empty()) {
            wstring current_path = dirs_to_process.front();
            dirs_to_process.pop_front();

            wstring search_path = current_path + L"\\*";
            HANDLE hFind = FindFirstFileW(search_path.c_str(), &ffd);

            if (hFind == INVALID_HANDLE_VALUE) {
                DWORD err = GetLastError();
                if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
                    wcerr << L"Ошибка доступа: " << search_path << endl;
                }
                continue;
            }

            do {
                if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                    continue;
                }

                wstring full_path = current_path + L"\\" + ffd.cFileName;
                file_info file;

                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    file.full_path = full_path;
                    file.name = ffd.cFileName;
                    file.directory = true;
                    file.extension = L"None";
                    file.size = calc_size(full_path);
                    info.push_back(file);
                    dirs_to_process.push_back(full_path);
                }
                else {
                    file.name = ffd.cFileName;
                    file.full_path = full_path;
                    file.directory = false;

                    HANDLE hFile = CreateFileW(
                        full_path.c_str(),
                        GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        DWORD filesize = GetFileSize(hFile, NULL);
                        file.size = filesize;
                    }
                    size_t pos = file.name.find_last_of('.'); // ищем индекс точки
                    if (pos != string::npos) // проверям, найдена ли точка. Если точка найдена, то делается срез символов после точки - расширение
                        // Если не найдена, то инструкция npos запишет в pos максимальный размер числа, которй может принять size_t и запишет в file.extension None
                    {
                        file.extension = file.name.substr(pos);
                    }
                    else
                    {
                        file.extension = L"None";
                    }

                    info.push_back(file);
                }
            } while (FindNextFileW(hFind, &ffd));

            FindClose(hFind);
        }
    }

    Iterator<T> begin() { return Iterator<T>(info.data(), info.data() + info.size()); }
    Iterator<T> end() { return Iterator<T>(info.data() + info.size(), info.data() + info.size()); }
};

#pragma pack(pop)

int main() {
    // Настройка консоли для вывода Unicode
    setlocale(LC_ALL, "RUSSIAN");

    Directory_It<file_info> it(L"C:\\Users\\yaros\\Desktop\\-FileSystem\\TestDirectory");

    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_MAXIMIZE);

    wcout << left << setw(24) << L"File Name" << L"Size" << L"     " << L"Extension" << L"         "
          << L"Directory?" << L"                                                      " << L"Full Path" << endl;

    for (auto i = it.begin(); i != it.end(); ++i) {
        file_info& item = *i;
        wcout << left << setw(25) << item.name << left << item.size << left << " " << setw(10) << left << setw(20) << item.extension << left << setw(15) << item.directory
              << L" " << item.full_path << endl;
    }

    return 0;
}