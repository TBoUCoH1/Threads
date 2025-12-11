#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <iomanip>
#include "DynamicArray.h"
#include "Map.h"

using namespace std;

enum class Grade { JUNIOR, MIDDLE, SENIOR };

ostream& operator<<(ostream& os, const Grade& g) {
    switch(g) {
        case Grade::JUNIOR: os << "Junior"; break;
        case Grade::MIDDLE: os << "Middle"; break;
        case Grade::SENIOR: os << "Senior"; break;
    }
    return os;
}

struct Date { int day, month, year; };

struct Employee {
    string fio;
    char gender; 
    Date birth_date;
    Grade grade;
};

struct GradeStats {
    long long total_age = 0;
    long long count = 0;
};

int calculate_age(const Date& birth, const Date& current) {
    int age = current.year - birth.year;
    if (current.month < birth.month || (current.month == birth.month && current.day < birth.day)) {
        age--;
    }
    return age;
}

DynamicArray<Employee> generate_data(int size) {
    DynamicArray<Employee> data(size);
    srand(time(nullptr)); 
    for (int i = 0; i < size; ++i) {
        Grade g = static_cast<Grade>(rand() % 3);
        data.push_back({ "User_" + to_string(i), 'M', {1, 1, 1970 + rand() % 40}, g });
    }
    return data;
}

void worker(DynamicArray<Employee>& data, int start, int end, Map<Grade, GradeStats>& local_map, const Date& current_date) {
    for (int i = start; i < end; ++i) {
        const Employee& emp = data[i];
        int age = calculate_age(emp.birth_date, current_date);
        
        local_map[emp.grade].total_age += age;
        local_map[emp.grade].count++;
    }
}

Map<Grade, GradeStats> process_single(DynamicArray<Employee>& data, const Date& current_date) {
    Map<Grade, GradeStats> result_map;
    int size = data.get_size();
    for(int i = 0; i < size; ++i) {
        const Employee& emp = data[i];
        int age = calculate_age(emp.birth_date, current_date);
        result_map[emp.grade].total_age += age;
        result_map[emp.grade].count++;
    }
    return result_map;
}

int main() {
    int N_elements = 0;
    int N_threads = 0;

    cout << "ЗАДАНИЕ 2" << endl;
    cout << "Введите размер массива (например 500000): ";
    if (!(cin >> N_elements)) N_elements = 500000;
    cout << "Введите кол-во потоков: ";
    if (!(cin >> N_threads)) N_threads = 4;

    Date now = {7, 12, 2025};
    cout << "\nГенерация данных..." << endl;
    DynamicArray<Employee> data = generate_data(N_elements);

    cout << "Обработка в 1 поток..." << endl;
    auto start_s = chrono::high_resolution_clock::now();
    
    Map<Grade, GradeStats> res_single = process_single(data, now);
    
    auto end_s = chrono::high_resolution_clock::now();
    cout << "Время (Single): " << chrono::duration<double, milli>(end_s - start_s).count() << " ms" << endl;

    cout << "Обработка в " << N_threads << " потоков..." << endl;
    
    DynamicArray<thread> threads(N_threads);
    DynamicArray<Map<Grade, GradeStats>> partial_maps(N_threads);
    
    for(int i=0; i<N_threads; ++i) partial_maps.push_back(Map<Grade, GradeStats>());

    auto start_m = chrono::high_resolution_clock::now();
    int chunk = N_elements / N_threads;

    for (int i = 0; i < N_threads; ++i) {
        int start = i * chunk;
        int end = (i == N_threads - 1) ? N_elements : (i + 1) * chunk;
        threads.emplace_back(worker, ref(data), start, end, ref(partial_maps[i]), now);
    }

    Map<Grade, GradeStats> final_map;
    for (int i = 0; i < N_threads; ++i) {
        if(threads[i].joinable()) threads[i].join();
        
        for(auto it = partial_maps[i].begin(); it != partial_maps[i].end(); ++it) {
            Grade g = it -> key;
            GradeStats stats = it -> value;
            final_map[g].total_age += stats.total_age;
            final_map[g].count += stats.count;
        }
    }

    auto end_m = chrono::high_resolution_clock::now();
    cout << "Время (Multi):  " << chrono::duration<double, milli>(end_m - start_m).count() << " ms" << endl;

    cout << "\nИТОГИ" << endl;
    for(auto it = final_map.begin(); it != final_map.end(); ++it) {
        if (it -> value.count > 0) {
            cout << left << setw(10) << it -> key
                 << ": " << fixed << setprecision(2) 
                 << (double)it -> value.total_age / it -> value.count << " лет" << endl;
        }
    }

    return 0;
}
