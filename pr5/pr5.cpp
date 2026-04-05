#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <mpi.h>

using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        cout << "      PARALLEL ARRAY SORTING WITH MPI" << endl;
        cout << "Program running on " << size << " processes" << endl;
        cout << endl;
    }

    vector<int> global_array;
    vector<int> local_array;
    int N = 0;

    if (rank == 0) {
        cout << "Enter array size N: ";
        while (!(cin >> N) || N <= 0) {
            cout << "Error! Enter a positive number: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        srand(static_cast<unsigned>(time(nullptr)));
        global_array.resize(N);

        cout << "\nGenerated array (" << N << " elements):" << endl;
        for (int i = 0; i < N; i++) {
            global_array[i] = rand() % 100;
            cout << global_array[i] << " ";
            if ((i + 1) % 15 == 0) cout << endl;
        }
        if (N % 15 != 0) cout << endl;
        cout << endl;
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int base_size = N / size;
    int remainder = N % size;
    int local_size = base_size + (rank < remainder ? 1 : 0);

    vector<int> send_counts(size);
    vector<int> displacements(size);

    if (rank == 0) {
        int displacement = 0;
        for (int i = 0; i < size; i++) {
            send_counts[i] = base_size + (i < remainder ? 1 : 0);
            displacements[i] = displacement;
            displacement += send_counts[i];
        }

        cout << "\nArray distribution between processes:" << endl;
        for (int i = 0; i < size; i++) {
            cout << "Process " << i << ": " << send_counts[i]
                 << " elements (offset: " << displacements[i] << ")" << endl;
        }
        cout << endl;
    }

    MPI_Bcast(send_counts.data(), size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displacements.data(), size, MPI_INT, 0, MPI_COMM_WORLD);

    local_array.resize(local_size);

    MPI_Scatterv(
        (rank == 0) ? global_array.data() : nullptr,
        send_counts.data(),
        displacements.data(),
        MPI_INT,
        local_array.data(),
        local_size,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    sort(local_array.begin(), local_array.end());

    cout << "Process " << rank << " sorted fragment of size " << local_size << endl;

    vector<int> sorted_global;
    if (rank == 0) {
        sorted_global.resize(N);
    }

    MPI_Gatherv(
        local_array.data(),
        local_size,
        MPI_INT,
        (rank == 0) ? sorted_global.data() : nullptr,
        send_counts.data(),
        displacements.data(),
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        cout << "\n                     RESULTS" << endl;

        for (int i = 0; i < size; i++) {
            int frag_size = send_counts[i];
            int start_idx = displacements[i];

            cout << "\nProcess " << i << " (offset " << start_idx
                 << ", size " << frag_size << "):" << endl;
            cout << "  ";
            for (int j = 0; j < frag_size; j++) {
                cout << sorted_global[start_idx + j];
                if (j < frag_size - 1) cout << " ";
            }
            cout << endl;
        }

        cout << "\n           ENTIRE ASSEMBLED ARRAY" << endl;
        cout << "\nTotal " << N << " elements:" << endl;
        for (int i = 0; i < N; i++) {
            cout << sorted_global[i] << " ";
            if ((i + 1) % 15 == 0) cout << endl;
        }
        if (N % 15 != 0) cout << endl;

        bool is_sorted = true;
        for (int i = 1; i < N; i++) {
            if (sorted_global[i] < sorted_global[i - 1]) {
                is_sorted = false;
                break;
            }
        }

        if (is_sorted) {
            cout << "\nSUCCESS: Array is correctly sorted!" << endl;
        } else {
            cout << "\nWARNING: Array may not be properly sorted!" << endl;
        }

        cout << "Program executed on " << size << " processes" << endl;

        cout << "\nPress Enter to exit..." << endl;
        cin.ignore();
        cin.get();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}