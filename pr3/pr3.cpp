#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <omp.h>

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

struct TimingResult {
    string mode;
    int array_size, repetitions, threads;
    double time_a, time_b, time_c, time_d, time_b_redef, time_d_redef, total_time;
};

int main() {
    setlocale(LC_ALL, "Russian");

    cout << "Доступно ядер: " << omp_get_num_procs() << endl;

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

    auto thread_counts = readNumbers("k = ");
    if (thread_counts.empty()) {
        thread_counts = { 20 };
    }

    vector<TimingResult> results;

    // ПОСЛЕДОВАТЕЛЬНАЯ ВЕРСИЯ
    cout << fixed << setprecision(2);
    cout << "TYPE\tN\tL\tThreads\ta_time\tb_time\tc_time\td_time\tb2_time\td2_time\ttotal_time\n";

    for (int current_N : N_values) {
        for (int current_L : L_values) {
            vector<double> array_a(current_N), array_b(current_N), array_c(current_N), array_d(current_N);

            auto start_time = chrono::high_resolution_clock::now();

            // Инициализация массива a
            array_a[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    accumulator += sin(x * i) + (x + i) / static_cast<double>(current_N);
                }
                array_a[i] = accumulator;
            }
            auto time_after_a = chrono::high_resolution_clock::now();

            // Инициализация массива b
            array_b[0] = 0.0;
            for (int i = 1; i < current_N; ++i) {
                double temp_value = array_b[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    temp_value += i * 10.0;
                }
                array_b[i] = temp_value;
            }
            auto time_after_b = chrono::high_resolution_clock::now();

            // Инициализация массива c
            for (int i = 0; i < current_N; ++i) {
                int reverse_index = current_N - 1 - i;
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    double divisor = current_N - i;
                    if (abs(divisor) < 1e-12) {
                        accumulator += 0.0;
                    } else {
                        accumulator += (array_a[i] - array_a[reverse_index]) / divisor;
                    }
                }
                array_c[i] = accumulator;
            }
            auto time_after_c = chrono::high_resolution_clock::now();

            // Инициализация массива d
            array_d[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_d[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    double denominator = current_val;
                    if (abs(denominator) < 1e-12) {
                        current_val = cos(0.0) + 0.0;
                    } else {
                        current_val = cos(i / denominator) + current_val / static_cast<double>(i);
                    }
                }
                array_d[i] = current_val;
            }
            auto time_after_d_init = chrono::high_resolution_clock::now();

            // Переопределение массива b
            array_b[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_b[i];
                for (int rep = 0; rep < current_L; ++rep) {
                    current_val = cos(current_val) + array_a[i];
                }
                array_b[i] = current_val;
            }
            auto time_after_b_redef = chrono::high_resolution_clock::now();

            // Переопределение массива d
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

            cout << "Seq\t" << current_N << "\t"
                 << current_L << "\t"
                 << "1\t"
                 << duration_a << "\t"
                 << duration_b << "\t"
                 << duration_c << "\t"
                 << duration_d << "\t"
                 << duration_b_redef << "\t"
                 << duration_d_redef << "\t"
                 << total_duration << "\n";

            results.push_back({ "Seq", current_N, current_L, 1,
                duration_a, duration_b, duration_c, duration_d,
                duration_b_redef, duration_d_redef, total_duration });
        }
    }

    // ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ (с parallel for)
    omp_set_num_threads(20);
    for (int current_N : N_values) {
        for (int current_L : L_values) {
            vector<double> array_a(current_N), array_b(current_N), array_c(current_N), array_d(current_N);

            auto start_time = chrono::high_resolution_clock::now();

            // Инициализация массива a
            array_a[0] = x;
#pragma omp parallel for
            for (int i = 1; i < current_N; ++i) {
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    accumulator += sin(x * i) + (x + i) / static_cast<double>(current_N);
                }
                array_a[i] = accumulator;
            }
            auto time_after_a = chrono::high_resolution_clock::now();

            // Инициализация массива b
            array_b[0] = 0.0;
            for (int i = 1; i < current_N; ++i) {
                double temp_value = array_b[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    temp_value += i * 10.0;
                }
                array_b[i] = temp_value;
            }
            auto time_after_b = chrono::high_resolution_clock::now();

            // Инициализация массива c
#pragma omp parallel for
            for (int i = 0; i < current_N; ++i) {
                int reverse_index = current_N - 1 - i;
                double accumulator = 0.0;
                for (int rep = 0; rep < current_L; ++rep) {
                    double divisor = current_N - i;
                    if (abs(divisor) < 1e-12) {
                        accumulator += 0.0;
                    } else {
                        accumulator += (array_a[i] - array_a[reverse_index]) / divisor;
                    }
                }
                array_c[i] = accumulator;
            }
            auto time_after_c = chrono::high_resolution_clock::now();

            // Инициализация массива d
            array_d[0] = x;
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_d[i - 1];
                for (int rep = 0; rep < current_L; ++rep) {
                    double denominator = current_val;
                    if (abs(denominator) < 1e-12) {
                        current_val = cos(0.0) + 0.0;
                    } else {
                        current_val = cos(i / denominator) + current_val / static_cast<double>(i);
                    }
                }
                array_d[i] = current_val;
            }
            auto time_after_d_init = chrono::high_resolution_clock::now();

            // Переопределение массива b
            array_b[0] = x;
#pragma omp parallel for
            for (int i = 1; i < current_N; ++i) {
                double current_val = array_b[i];
                for (int rep = 0; rep < current_L; ++rep) {
                    current_val = cos(current_val) + array_a[i];
                }
                array_b[i] = current_val;
            }
            auto time_after_b_redef = chrono::high_resolution_clock::now();

            // Переопределение массива d
#pragma omp parallel for
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

            cout << "Parallel\t" << current_N << "\t"
                 << current_L << "\t"
                 << "20\t"
                 << duration_a << "\t"
                 << duration_b << "\t"
                 << duration_c << "\t"
                 << duration_d << "\t"
                 << duration_b_redef << "\t"
                 << duration_d_redef << "\t"
                 << total_duration << "\n";

            results.push_back({ "Parallel", current_N, current_L, 20,
                duration_a, duration_b, duration_c, duration_d,
                duration_b_redef, duration_d_redef, total_duration });
        }
    }

    // ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ С parallel sections
    for (int k : thread_counts) {
        omp_set_num_threads(k);

        for (int current_N : N_values) {
            for (int current_L : L_values) {
                vector<double> array_a(current_N), array_b(current_N), array_c(current_N), array_d(current_N);

                auto start_time = chrono::high_resolution_clock::now();

                double a_time = 0.0, b_time = 0.0, d_time = 0.0;
                double c_time = 0.0, b2_time = 0.0;
                double d2_time = 0.0;

                // ФАЗА 1: Параллельное выполнение a, b, d (3 секции)
                auto phase1_start = chrono::high_resolution_clock::now();

#pragma omp parallel sections
                {
#pragma omp section
                    {
                        auto section_start = chrono::high_resolution_clock::now();
                        array_a[0] = x;
                        for (int i = 1; i < current_N; ++i) {
                            double accumulator = 0.0;
                            for (int rep = 0; rep < current_L; ++rep) {
                                accumulator += sin(x * i) + (x + i) / static_cast<double>(current_N);
                            }
                            array_a[i] = accumulator;
                        }
                        auto section_end = chrono::high_resolution_clock::now();
                        a_time = chrono::duration<double>(section_end - section_start).count();
                    }

#pragma omp section
                    {
                        auto section_start = chrono::high_resolution_clock::now();
                        array_b[0] = 0.0;
                        for (int i = 1; i < current_N; ++i) {
                            double temp_value = array_b[i - 1];
                            for (int rep = 0; rep < current_L; ++rep) {
                                temp_value += i * 10.0;
                            }
                            array_b[i] = temp_value;
                        }
                        auto section_end = chrono::high_resolution_clock::now();
                        b_time = chrono::duration<double>(section_end - section_start).count();
                    }

#pragma omp section
                    {
                        auto section_start = chrono::high_resolution_clock::now();
                        array_d[0] = x;
                        for (int i = 1; i < current_N; ++i) {
                            double current_val = array_d[i - 1];
                            for (int rep = 0; rep < current_L; ++rep) {
                                double denominator = current_val;
                                if (abs(denominator) < 1e-12) {
                                    current_val = cos(0.0) + 0.0;
                                } else {
                                    current_val = cos(i / denominator) + current_val / static_cast<double>(i);
                                }
                            }
                            array_d[i] = current_val;
                        }
                        auto section_end = chrono::high_resolution_clock::now();
                        d_time = chrono::duration<double>(section_end - section_start).count();
                    }
                }
                auto phase1_end = chrono::high_resolution_clock::now();

                // ФАЗА 2: Параллельное выполнение c и переопределение b (b2)
                auto phase2_start = chrono::high_resolution_clock::now();

#pragma omp parallel sections
                {
#pragma omp section
                    {
                        auto section_start = chrono::high_resolution_clock::now();
                        for (int i = 0; i < current_N; ++i) {
                            int reverse_index = current_N - 1 - i;
                            double accumulator = 0.0;
                            for (int rep = 0; rep < current_L; ++rep) {
                                double divisor = current_N - i;
                                if (abs(divisor) < 1e-12) {
                                    accumulator += 0.0;
                                } else {
                                    accumulator += (array_a[i] - array_a[reverse_index]) / divisor;
                                }
                            }
                            array_c[i] = accumulator;
                        }
                        auto section_end = chrono::high_resolution_clock::now();
                        c_time = chrono::duration<double>(section_end - section_start).count();
                    }

#pragma omp section
                    {
                        auto section_start = chrono::high_resolution_clock::now();
                        array_b[0] = x;
                        for (int i = 1; i < current_N; ++i) {
                            double current_val = array_b[i];
                            for (int rep = 0; rep < current_L; ++rep) {
                                current_val = cos(current_val) + array_a[i];
                            }
                            array_b[i] = current_val;
                        }
                        auto section_end = chrono::high_resolution_clock::now();
                        b2_time = chrono::duration<double>(section_end - section_start).count();
                    }
                }
                auto phase2_end = chrono::high_resolution_clock::now();

                // ФАЗА 3: Последовательное переопределение массива d (d2)
                auto phase3_start = chrono::high_resolution_clock::now();
                for (int i = 0; i < current_N; ++i) {
                    int reverse_index = current_N - 1 - i;
                    double accumulator = 0.0;
                    for (int rep = 0; rep < current_L; ++rep) {
                        accumulator += array_a[i] * array_c[reverse_index] / 2.0;
                    }
                    array_d[i] = accumulator;
                }
                auto phase3_end = chrono::high_resolution_clock::now();

                d2_time = chrono::duration<double>(phase3_end - phase3_start).count();
                auto end_time = chrono::high_resolution_clock::now();

                double phase1_duration = max({ a_time, b_time, d_time });
                double phase2_duration = max(c_time, b2_time);

                double total_duration = chrono::duration<double>(end_time - start_time).count();

                cout << "Sections\t" << current_N << "\t"
                     << current_L << "\t"
                     << k << "\t"
                     << a_time << "\t"
                     << b_time << "\t"
                     << c_time << "\t"
                     << d_time << "\t"
                     << b2_time << "\t"
                     << d2_time << "\t"
                     << total_duration << "\n";

                results.push_back({ "Sections", current_N, current_L, k,
                    a_time, b_time, c_time, d_time,
                    b2_time, d2_time, total_duration });
            }
        }
    }

    ofstream output_file("results.txt");
    if (output_file.is_open()) {
        output_file << fixed << setprecision(2);
        output_file << "TYPE\tN\tL\tThreads\ta_time\tb_time\tc_time\td_time\tb2_time\td2_time\ttotal_time\n";
        for (const auto& result : results) {
            output_file << result.mode << "\t"
                        << result.array_size << "\t"
                        << result.repetitions << "\t"
                        << result.threads << "\t"
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
    } else {
        cerr << "Ошибка: не удалось создать файл results.txt" << endl;
    }

    return 0;
}