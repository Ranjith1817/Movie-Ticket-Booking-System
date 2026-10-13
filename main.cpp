#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include <limits>
#include <cctype
using namespace std;

const int ROWS = 5;
const int COLS = 8;

struct Booking {
    int id;
    string customer;
    string movie;
    string showtime;
    int row;
    int col;
    bool active;
};

struct BookingNode {
    Booking data;
    BookingNode* next;
    BookingNode(const Booking& b) : data(b), next(nullptr) {}
};

struct MovieNode {
    string name;
    vector<string> shows;
    MovieNode* next;
    MovieNode(string n, vector<string> s) : name(std::move(n)), shows(std::move(s)), next(nullptr) {}
};

class MovieTicketSystem {
private:
    int seats[ROWS][COLS]{};

    MovieNode* moviesHead = nullptr;
    BookingNode* bookingsHead = nullptr;

    queue<Booking> requests;

    stack<int> undoStack;

    int nextBookingId = 1001;

    static string lower(string s) {
        transform(s.begin(), s.end(), s.begin(),
                  [](unsigned char c) { return static_cast<char>(tolower(c)); });
        return s;
    }

    MovieNode* findMovie(const string& name) {
        for (MovieNode* p = moviesHead; p; p = p->next)
            if (lower(p->name) == lower(name)) return p;
        return nullptr;
    }

    BookingNode* findBooking(int id) {
        for (BookingNode* p = bookingsHead; p; p = p->next)
            if (p->data.id == id) return p;
        return nullptr;
    }

    void addBookingRecord(const Booking& b) {
        BookingNode* node = new BookingNode(b);
        node->next = bookingsHead;
        bookingsHead = node;
    }

    void showMovies() {
        cout << "\nAvailable movies and showtimes:\n";
        int index = 1;
        for (MovieNode* p = moviesHead; p; p = p->next) {
            cout << index++ << ". " << p->name << " | Shows: ";
            for (size_t i = 0; i < p->shows.size(); ++i) {
                if (i) cout << ", ";
                cout << p->shows[i];
            }
            cout << '\n';
        }
    }

    void showSeats() {
        cout << "\nSeat map (O = Available, X = Booked)\n    ";
        for (int c = 0; c < COLS; ++c) cout << c + 1 << ' ';
        cout << '\n';
        for (int r = 0; r < ROWS; ++r) {
            cout << "Row " << char('A' + r) << " ";
            for (int c = 0; c < COLS; ++c)
                cout << (seats[r][c] ? "X " : "O ");
            cout << '\n';
        }
    }

    void enqueueDemoRequest() {
        string customer, movie, showtime;
        int row, col;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Customer name: ";
        getline(cin, customer);

        showMovies();
        cout << "Enter movie name exactly as shown: ";
        getline(cin, movie);
        MovieNode* m = findMovie(movie);
        if (!m) {
            cout << "Movie not found.\n";
            return;
        }

        cout << "Enter showtime exactly as shown: ";
        getline(cin, showtime);
        bool validShow = false;
        for (const auto& s : m->shows)
            if (lower(s) == lower(showtime)) validShow = true;
        if (!validShow) {
            cout << "Showtime not found for this movie.\n";
            return;
        }

        showSeats();
        cout << "Enter row letter (A-" << char('A' + ROWS - 1) << "): ";
        char rowChar;
        cin >> rowChar;
        rowChar = static_cast<char>(toupper(static_cast<unsigned char>(rowChar)));
        cout << "Enter seat number (1-" << COLS << "): ";
        cin >> col;
        row = rowChar - 'A';

        if (row < 0 || row >= ROWS || col < 1 || col > COLS) {
            cout << "Invalid seat. Request rejected.\n";
            return;
        }

        Booking b{nextBookingId++, customer, m->name, showtime, row, col - 1, true};
        requests.push(b);
        cout << "Request added to queue. Processing FIFO...\n";

        // Process queued requests in order. Availability is checked at processing time.
        while (!requests.empty()) {
            Booking request = requests.front();
            requests.pop();

            if (seats[request.row][request.col] == 1) {
                cout << "Request for " << char('A' + request.row) << request.col + 1
                     << " rejected: seat is already booked.\n";
                continue;
            }

            seats[request.row][request.col] = 1;
            addBookingRecord(request);
            undoStack.push(request.id);
            cout << "Booking confirmed! Booking ID: " << request.id
                 << " | " << request.movie << " | " << request.showtime
                 << " | Seat " << char('A' + request.row) << request.col + 1 << '\n';
        }
    }

    void listBookings() {
        cout << "\nBooking records (linked list):\n";
        bool found = false;
        for (BookingNode* p = bookingsHead; p; p = p->next) {
            if (!p->data.active) continue;
            found = true;
            cout << "ID " << p->data.id << " | " << p->data.customer
                 << " | " << p->data.movie << " | " << p->data.showtime
                 << " | Seat " << char('A' + p->data.row) << p->data.col + 1 << '\n';
        }
        if (!found) cout << "No active bookings.\n";
    }

    void cancelBooking() {
        int id;
        cout << "Enter booking ID to cancel: ";
        cin >> id;
        BookingNode* node = findBooking(id);
        if (!node || !node->data.active) {
            cout << "Booking ID not found or already cancelled.\n";
            return;
        }

        node->data.active = false;
        seats[node->data.row][node->data.col] = 0;
        cout << "Booking " << id << " cancelled; seat released.\n";
        // Remove this ID from the undo stack if it is at the top.
        if (!undoStack.empty() && undoStack.top() == id) undoStack.pop();
    }

    void undoLastBooking() {
        if (undoStack.empty()) {
            cout << "No recent booking to undo.\n";
            return;
        }
        int id = undoStack.top();
        undoStack.pop();
        BookingNode* node = findBooking(id);
        if (!node || !node->data.active) {
            cout << "Latest booking is already inactive.\n";
            return;
        }
        node->data.active = false;
        seats[node->data.row][node->data.col] = 0;
        cout << "Undid most recent booking ID " << id << ". Seat released.\n";
    }

    void searchMovie() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string term;
        cout << "Enter movie name to search: ";
        getline(cin, term);
        bool found = false;
        for (MovieNode* p = moviesHead; p; p = p->next) {
            if (lower(p->name).find(lower(term)) != string::npos) {
                cout << "Found: " << p->name << '\n';
                found = true;
            }
        }
        if (!found) cout << "No matching movie found.\n";
    }

    void sortMovies() {
        vector<string> names;
        for (MovieNode* p = moviesHead; p; p = p->next) names.push_back(p->name);
        sort(names.begin(), names.end());
        cout << "\nMovies sorted alphabetically:\n";
        for (const auto& name : names) cout << "- " << name << '\n';
    }

    void showTheatreGraph() {
        // Example graph: nodes are theatre locations; edges are connections.
        const int N = 5;
        const string places[N] = {"Alliance Theatre", "Anekal Theatre", "Central Theatre",
                                  "East Theatre", "West Theatre"};
        const int graph[N][N] = {
            {0, 1, 1, 0, 0},
            {1, 0, 0, 1, 0},
            {1, 0, 0, 1, 1},
            {0, 1, 1, 0, 1},
            {0, 0, 1, 1, 0}
        };
        bool visited[N]{};
        queue<int> q;
        q.push(0);
        visited[0] = true;
        cout << "\nGraph traversal (example connections, not live map data):\n";
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            cout << places[u] << '\n';
            for (int v = 0; v < N; ++v) {
                if (graph[u][v] && !visited[v]) {
                    visited[v] = true;
                    q.push(v);
                }
            }
        }
    }

    void seedMovies() {
        moviesHead = new MovieNode("Interstellar", {"10:00 AM", "2:00 PM", "6:00 PM"});
        moviesHead->next = new MovieNode("Inception", {"11:00 AM", "3:00 PM", "7:00 PM"});
        moviesHead->next->next = new MovieNode("Avengers: Endgame", {"9:30 AM", "1:30 PM", "5:30 PM"});
        moviesHead->next->next->next = new MovieNode("The Batman", {"10:30 AM", "2:30 PM", "6:30 PM"});
    }

    void cleanup() {
        while (moviesHead) {
            MovieNode* next = moviesHead->next;
            delete moviesHead;
            moviesHead = next;
        }
        while (bookingsHead) {
            BookingNode* next = bookingsHead->next;
            delete bookingsHead;
            bookingsHead = next;
        }
    }

public:
    MovieTicketSystem() { seedMovies(); }
    ~MovieTicketSystem() { cleanup(); }

    void run() {
        int choice;
        do {
            cout << "\n========== MOVIE TICKET BOOKING SYSTEM ==========\n"
                 << "1. View movies and showtimes\n"
                 << "2. View seat map\n"
                 << "3. Book a ticket\n"
                 << "4. View active bookings\n"
                 << "5. Cancel a booking\n"
                 << "6. Undo most recent booking\n"
                 << "7. Search movies\n"
                 << "8. Sort movies alphabetically\n"
                 << "9. Show theatre graph traversal (demo)\n"
                 << "0. Exit\n"
                 << "Choose an option: ";
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Please enter a number.\n";
                continue;
            }

            switch (choice) {
                case 1: showMovies(); break;
                case 2: showSeats(); break;
                case 3: enqueueDemoRequest(); break;
                case 4: listBookings(); break;
                case 5: cancelBooking(); break;
                case 6: undoLastBooking(); break;
                case 7: searchMovie(); break;
                case 8: sortMovies(); break;
                case 9: showTheatreGraph(); break;
                case 0: cout << "Thank you for using the system!\n"; break;
                default: cout << "Invalid option. Try again.\n";
            }
        } while (choice != 0);
    }
};

int main() {
    MovieTicketSystem system;
    system.run();
    return 0;
}
