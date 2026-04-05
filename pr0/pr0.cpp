#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

vector<int> readNumbers(const string& prompt) {
    cout << prompt;
    string input_line;
    getline(cin, input_line);
    if (input_line.empty()) {
        getline(cin, input_line);
    }

    istringstream stream(input_line);
    vector<int> numbers;
    int value;
    while (stream >> value) {
        numbers.push_back(value);
    }
    return numbers;
}

int main() {
    setlocale(LC_ALL, "Russian");

    auto N_values = readNumbers("N = ");
    auto L_values = readNumbers("L = ");

    if (N_values.empty() || L_values.empty()) {
        cout << "Ошибка: не введены значения для N или L.\n";
        return 1;
    }

    double x;
    cout << "x = ";
    cin >> x;
    cin.ignore();

    cout << fixed << setprecision(2);

    struct TimingResult {
        int array_size, repetitions;
        double time_a, time_b, time_c, time_d, time_b_redef, time_d_redef, total_time;
    };
    vector<TimingResult> results;

    for (int current_N : N_values) {
        for (int current_L : L_values) {

            vector<double> array_a(current_N), array_b(current_N), array_c(current_N), array_d(current_N);

            auto start_time = chrono::high_resolution_clock::now();

            // Инициализация массива a: a[0] = x; a[i] = sin(x * i) + (x + i) / n;
            array_a[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    accumulator += sin(x * i) + (x + i) / static_cast<double>(current_N);
                }
                array_a[i] = accumulator;
            }
            auto time_after_a = chrono::high_resolution_clock::now();

            // Инициализация массива b: b[0] = 0; b[i] = b[i - 1] + i*10;
            array_b[0] = 0.0;
            for (int i = 1; i < current_N; ++i) {
                double temp_value = array_b[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    temp_value += i * 10.0;
                }
                array_b[i] = temp_value;
            }
            auto time_after_b = chrono::high_resolution_clock::now();

            // Инициализация массива c: c[i] = (a[i] - a[n-i]) / (n - i);
            for (int i = 0; i < current_N; ++i) {
                int reverse_index = current_N - 1 - i;
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    double divisor = current_N - i;
                    if (abs(divisor) < 1e-12) {
                        accumulator += 0.0;
                    }
                    else {
                        accumulator += (array_a[i] - array_a[reverse_index]) / divisor;
                    }
                }
                array_c[i] = accumulator;
            }
            auto time_after_c = chrono::high_resolution_clock::now();

            // Инициализация массива d: d[0] = x; d[i] = cos(i / d[i - 1]) + d[i - 1] / i;
            array_d[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_d[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    double denominator = current_val;
                    if (abs(denominator) < 1e-12) {
                        current_val = cos(0.0) + 0.0;
                    }
                    else {
                        current_val = cos(i / denominator) + current_val / static_cast<double>(i);
                    }
                }
                array_d[i] = current_val;
            }
            auto time_after_d_init = chrono::high_resolution_clock::now();

            // Переопределение массива b: b[0] = x; b[i] = cos(b[i]) + a[i];
            array_b[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_b[i];
                for (int rep = 0; rep < current_L; ++rep) {
                    current_val = cos(current_val) + array_a[i];
                }
                array_b[i] = current_val;
            }
            auto time_after_b_redef = chrono::high_resolution_clock::now();

            // Переопределение массива d: d[i] = a[i] * c[n - i] / 2;
            for (int i = 0; i < current_N; ++i) {
                int reverse_index = current_N - 1 - i;
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    accumulator += array_a[i] * array_c[reverse_index] / 2.0;
                }
                array_d[i] = accumulator;
            }
            auto end_time = chrono::high_resolution_clock::now();

            double duration_a = chrono::duration<double>(time_after_a - start_time).count();
            double duration_b = chrono::duration<double>(time_after_b - time_after_a).count();
            double duration_c = chrono::duration<double>(time_after_c - time_after_b).count();
            double duration_d = chrono::duration<double>(time_after_d_init - time_after_c).count();
            double duration_b_redef = chrono::duration<double>(time_after_b_redef - time_after_d_init).count();
            double duration_d_redef = chrono::duration<double>(end_time - time_after_b_redef).count();
            double total_duration = chrono::duration<double>(end_time - start_time).count();

            cout << current_N << "\t"
                 << current_L << "\t"
                 << duration_a << "\t"
                 << duration_b << "\t"
                 << duration_c << "\t"
                 << duration_d << "\t"
                 << duration_b_redef << "\t"
                 << duration_d_redef << "\t"
                 << total_duration << "\n";

            results.push_back({ current_N, current_L, duration_a, duration_b, duration_c, duration_d,
                              duration_b_redef, duration_d_redef, total_duration });
        }
    }

    ofstream output_file("results.txt");
    if (output_file.is_open()) {
        output_file << fixed << setprecision(2);
        for (const auto& result : results) {
            output_file << result.array_size << "\t"
                        << result.repetitions << "\t"
                        << result.time_a << "\t"
                        << result.time_b << "\t"
                        << result.time_c << "\t"
                        << result.time_d << "\t"
                        << result.time_b_redef << "\t"
                        << result.time_d_redef << "\t"
                        << result.total_time << "\n";
        }
        output_file.close();
        cout << "\nРезультаты сохранены в файл results.txt" << endl;
    }
    else {
        cerr << "Ошибка: не удалось создать файл results.txt" << endl;
    }

    return 0;
}