#include <iostream>
#include <cmath>
#include <ctime>
#include <string>
#include <fstream>
#include <climits>
#include <iomanip>
#include "omp.h"

using namespace std;

struct Node {
    double data;
    Node* next;
};

class CircularList {
public:
    Node* head;
    double min_value;
    int count;

    CircularList() : head(nullptr), min_value(DBL_MAX), count(0) {}

    void AddSorted(double value, Node* pred = nullptr, Node* curr = nullptr) {
        Node* newNode = new Node{ value, nullptr };
        if (value < min_value) min_value = value;
        count++;

        if (!head) {
            head = newNode;
            head->next = head;
            return;
        }

        if (pred == nullptr && curr == nullptr) {
            pred = nullptr;
            curr = head;

            do {
                if (curr->data >= value)
                    break;
                pred = curr;
                curr = curr->next;
            } while (curr != head);

            AddSorted(value, pred, curr);
            return;
        }

        if (pred == nullptr) {
            Node* last = head;
            while (last->next != head) last = last->next;

            newNode->next = head;
            last->next = newNode;
            head = newNode;
            return;
        }

        newNode->next = curr;
        pred->next = newNode;
    }

    void Print() {
        if (!head) {
            cout << "Список пуст.\n";
            return;
        }
        Node* tmp = head;
        int i = 0;
        const int MAX_PRINT = 20;
        do {
            cout << fixed << setprecision(4) << tmp->data;
            tmp = tmp->next;
            i++;
            if (tmp != head && i < MAX_PRINT) cout << " -> ";
            if (i >= MAX_PRINT) {
                cout << " -> ... (еще " << count - MAX_PRINT << " элементов)";
                break;
            }
        } while (tmp != head && i < MAX_PRINT);
        cout << " -> (кольцо)\n";
    }

    double GetMin() {
        return (min_value == DBL_MAX) ? 0.0 : min_value;
    }

    int GetCount() {
        return count;
    }

    void Clear() {
        if (!head) return;
        Node* current = head->next;
        while (current != head) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        delete head;
        head = nullptr;
        min_value = DBL_MAX;
        count = 0;
    }

    ~CircularList() {
        Clear();
    }
};

int main() {
    setlocale(LC_ALL, "Russian");

    int N, g, k;
    cout << "Введите N: "; cin >> N;
    cout << "Введите g: "; cin >> g;
    cout << "Введите k: "; cin >> k;

    double* arr = new double[N];
    srand(time(NULL));
    for (int i = 0; i < N; ++i) {
        arr[i] = (double)rand() / RAND_MAX * 2.0 - 1.0;
    }

    ofstream screen_file("screen.txt");

    double time1_seq = 0.0, time2_seq = 0.0, time5_seq = 0.0;
    double time1_par = 0.0, time2_par = 0.0, time5_par = 0.0;

    cout << "\nПОСЛЕДОВАТЕЛЬНАЯ ВЕРСИЯ:\n";

    double seq_total_time = 0.0;
    CircularList seq_list;
    int* seq_hist = new int[g]();
    double seq_min_diff = DBL_MAX, seq_max_diff = -DBL_MAX;

    for (int i = 0; i < N - 1; ++i) {
        double diff = arr[i] - arr[i + 1];
        if (diff < seq_min_diff) seq_min_diff = diff;
        if (diff > seq_max_diff) seq_max_diff = diff;
    }

    if (seq_max_diff - seq_min_diff < 1e-10) {
        seq_max_diff = seq_min_diff + 1.0;
    }

    {
        double start_total = omp_get_wtime();

        double start1 = omp_get_wtime();
        for (int i = 0; i < N; ++i) {
            if (i % 2 == 0) {
                cout << "Последовательная: элемент [" << i << "] = "
                     << fixed << setprecision(4) << arr[i] << '\n';
                if (screen_file.is_open()) {
                    screen_file << "Последовательная: элемент [" << i << "] = "
                                << fixed << setprecision(4) << arr[i] << '\n';
                }
            }
        }
        time1_seq = omp_get_wtime() - start1;

        double start2 = omp_get_wtime();
        for (int i = 1; i < N; ++i) {
            if (fabs(arr[i]) > fabs(arr[i - 1])) {
                seq_list.AddSorted(arr[i]);
            }
        }
        time2_seq = omp_get_wtime() - start2;

        double start5 = omp_get_wtime();
        for (int i = 0; i < N - 1; ++i) {
            double diff = arr[i] - arr[i + 1];
            double normalized = (diff - seq_min_diff) / (seq_max_diff - seq_min_diff);
            int bin = static_cast<int>(normalized * g);
            if (bin < 0) bin = 0;
            if (bin >= g) bin = g - 1;
            seq_hist[bin]++;
        }
        time5_seq = omp_get_wtime() - start5;

        seq_total_time = omp_get_wtime() - start_total;
    }

    cout << "\nСписок (отсортированный): ";
    seq_list.Print();
    cout << "Минимум: " << fixed << setprecision(4) << seq_list.GetMin() << "\n";

    cout << "\nГистограмма разностей:\n";
    double seq_width = (seq_max_diff - seq_min_diff) / g;
    for (int i = 0; i < g; ++i) {
        double left = seq_min_diff + i * seq_width;
        double right = left + seq_width;
        cout << "[" << fixed << setprecision(3) << left << ", "
             << fixed << setprecision(3) << right << "): " << seq_hist[i] << "\n";
    }
    cout << "Время: " << seq_total_time << " сек\n";

    cout << "\nПАРАЛЛЕЛЬНАЯ ВЕРСИЯ (" << k << " потоков):\n";

    omp_set_num_threads(k);
    double par_total_time = 0.0;
    CircularList par_list;
    int* par_hist = new int[g]();
    double par_min_diff = DBL_MAX, par_max_diff = -DBL_MAX;

    {
        double start_total = omp_get_wtime();

#pragma omp parallel num_threads(k)
        {
            double local_min = DBL_MAX;
            double local_max = -DBL_MAX;

#pragma omp for schedule(static)
            for (int i = 0; i < N - 1; ++i) {
                double diff = arr[i] - arr[i + 1];
                if (diff < local_min) local_min = diff;
                if (diff > local_max) local_max = diff;
            }

#pragma omp critical(minmax)
            {
                if (local_min < par_min_diff) par_min_diff = local_min;
                if (local_max > par_max_diff) par_max_diff = local_max;
            }
        }

        if (par_max_diff - par_min_diff < 1e-10) {
            par_max_diff = par_min_diff + 1.0;
        }

        double start1_par = omp_get_wtime();
#pragma omp parallel num_threads(k)
        {
#pragma omp for schedule(static)
            for (int i = 0; i < N; ++i) {
                if (i % 2 == 0) {
#pragma omp critical(output)
                    {
                        cout << "Поток " << omp_get_thread_num()
                             << ": элемент [" << i << "] = "
                             << fixed << setprecision(4) << arr[i] << '\n';
                        if (screen_file.is_open()) {
                            screen_file << "Поток " << omp_get_thread_num()
                                        << ": элемент [" << i << "] = "
                                        << fixed << setprecision(4) << arr[i] << '\n';
                        }
                    }
                }
            }
        }
        time1_par = omp_get_wtime() - start1_par;

        double start2_par = omp_get_wtime();

#pragma omp parallel num_threads(k)
        {
#pragma omp for schedule(static)
            for (int i = 1; i < N; ++i) {
                if (fabs(arr[i]) <= fabs(arr[i - 1]))
                    continue;

                double value = arr[i];

                Node* pred = nullptr;
                Node* curr = nullptr;

                if (par_list.head) {
                    pred = nullptr;
                    curr = par_list.head;

                    do {
                        if (curr->data >= value)
                            break;
                        pred = curr;
                        curr = curr->next;
                    } while (curr != par_list.head);
                }

#pragma omp critical(list_insert)
                {
                    if (!par_list.head) {
                        par_list.AddSorted(value, nullptr, nullptr);
                    }
                    else {
                        bool prediction_valid = true;

                        if (pred == nullptr) {
                            if (par_list.head != curr) prediction_valid = false;
                        }
                        else {
                            if (pred->next != curr) prediction_valid = false;
                        }

                        if (prediction_valid) {
                            par_list.AddSorted(value, pred, curr);
                        }
                        else {
                            Node* p = nullptr;
                            Node* c = par_list.head;
                            do {
                                if (c->data >= value)
                                    break;
                                p = c;
                                c = c->next;
                            } while (c != par_list.head);

                            par_list.AddSorted(value, p, c);
                        }
                    }
                }
            }
        }
        time2_par = omp_get_wtime() - start2_par;

        double start5_par = omp_get_wtime();
#pragma omp parallel num_threads(k)
        {
#pragma omp for schedule(static)
            for (int i = 0; i < N - 1; ++i) {
                double diff = arr[i] - arr[i + 1];
                double normalized = (diff - par_min_diff) / (par_max_diff - par_min_diff);
                int bin = static_cast<int>(normalized * g);
                if (bin < 0) bin = 0;
                if (bin >= g) bin = g - 1;
#pragma omp atomic
                par_hist[bin]++;
            }
        }
        time5_par = omp_get_wtime() - start5_par;

        par_total_time = omp_get_wtime() - start_total;
    }

    cout << "\nСписок (отсортированный): ";
    par_list.Print();
    cout << "Минимум: " << fixed << setprecision(4) << par_list.GetMin() << "\n";

    cout << "\nГистограмма разностей:\n";
    double par_width = (par_max_diff - par_min_diff) / g;
    for (int i = 0; i < g; ++i) {
        double left = par_min_diff + i * par_width;
        double right = left + par_width;
        cout << "[" << fixed << setprecision(3) << left << ", "
             << fixed << setprecision(3) << right << "): " << par_hist[i] << "\n";
    }
    cout << "Время: " << par_total_time << " сек\n";

    cout << "\nТАБЛИЦА ВРЕМЕНИ:\n";
    cout << "Пункт          N     g     k     Время (сек)\n";

    cout << "1 (посл)       " << N << "     " << g << "     1     "
         << fixed << setprecision(6) << time1_seq << "\n";
    cout << "2 (посл)       " << N << "     " << g << "     1     "
         << time2_seq << "\n";
    cout << "5 (посл)       " << N << "     " << g << "     1     "
         << time5_seq << "\n";
    cout << "Общ (посл)     " << N << "     " << g << "     1     "
         << seq_total_time << "\n";

    cout << "1 (пар)        " << N << "     " << g << "     " << k << "     "
         << time1_par << "\n";
    cout << "2 (пар)        " << N << "     " << g << "     " << k << "     "
         << time2_par << "\n";
    cout << "5 (пар)        " << N << "     " << g << "     " << k << "     "
         << time5_par << "\n";
    cout << "Общ (пар)      " << N << "     " << g << "     " << k << "     "
         << par_total_time << "\n";

    cout << "\nСРАВНЕНИЕ:\n";
    double speedup_total = (par_total_time > 0) ? seq_total_time / par_total_time : 0.0;

    if (speedup_total > 1.0) {
        cout << "Параллельная БЫСТРЕЕ на " << fixed << setprecision(1)
             << ((speedup_total - 1.0) * 100) << "%\n";
    }
    else if (speedup_total < 1.0 && speedup_total > 0) {
        cout << "Параллельная МЕДЛЕННЕЕ на " << fixed << setprecision(1)
             << ((1.0 / speedup_total - 1.0) * 100) << "%\n";
    }
    else {
        cout << "Версии работают одинаково\n";
    }

    if (screen_file.is_open()) screen_file.close();
    seq_list.Clear();
    par_list.Clear();
    delete[] arr;
    delete[] seq_hist;
    delete[] par_hist;

    return 0;
}