#include <iostream>   // For input and output
#include <vector>     // For vectors and matrices
#include <string>     // For string operations
#include <fstream>    // For saving data to files
#include <iomanip>    // For formatting output
#include <algorithm>  // For ordering and sorting
#include <limits>     // For numeric_limits
#include <direct.h>   // For directory creation

using namespace std;

/*
 * Rwanda Infrastructure Management System
 *
 * A console application to manage cities, roads, and budgets for Rwanda's infrastructure,
 * under the Ministry of Infrastructure (MININFRA). This application supports adding cities,
 * roads, and budgets; editing city names; searching cities by index; displaying data; and
 * persisting data to files in the "data" directory. Data is loaded from files on startup and
 * saved immediately after each operation for live synchronization.
 *
 * Author: [Ishimwe Arsene]
 * Date: [23.05.2025]
 */

class InfrastructureManager {
private:
    vector<string> city_names;          // Stores city names
    vector<vector<int>> roads;          // Adjacency matrix indicating roads between cities (1 if road exists, 0 otherwise)
    vector<vector<double>> budgets;     // Adjacency matrix holding budgets for roads between cities
    const int MAX_CITIES = 500;         // Maximum number of cities to prevent crashes

    // Returns 0-based index of city by name, or -1 if not found
    int get_city_index(const string& name) {
        for (size_t i = 0; i < city_names.size(); i++) {
            if (city_names[i] == name) return static_cast<int>(i);
        }
        return -1;
    }

    // Checks if a city exists
    bool city_exists(const string& name) {
        return get_city_index(name) != -1;
    }

    // Validates city name
    bool is_valid_city_name(const string& name) {
        if (name.empty() || name.length() < 2) {
            return false;
        }
        bool has_letter = false;
        for (char c : name) {
            if (isalpha(c)) has_letter = true;
            if (!isalnum(c) && c != ' ' && c != '-') {
                return false;
            }
        }
        return has_letter;
    }

    // Validates budget amount
    bool is_valid_budget(double budget) {
        return budget > 0 && budget <= 1000.0; // Max 1000 billion RWF
    }

    // Validates city count
    bool is_valid_city_count(int count) {
        return count > 0 && count <= MAX_CITIES - static_cast<int>(city_names.size());
    }

    // Validates city index
    bool is_valid_index(int index) {
        return index >= 1 && index <= static_cast<int>(city_names.size());
    }

    // Creates data directory if it doesn't exist
    void ensure_data_directory() {
        #ifdef _WIN32
            _mkdir("data");
        #else
            mkdir("data", 0777);
        #endif
    }

    // Saves cities to data/cities.txt
    void save_cities_to_file() {
        ensure_data_directory();
        ofstream cities_file("data/cities.txt");
        if (cities_file.is_open()) {
            cities_file << "Index\tCity Name\n";
            for (size_t i = 0; i < city_names.size(); i++) {
                cities_file << (i + 1) << "\t" << city_names[i] << "\n";
            }
            cities_file.close();
        } else {
            cout << "Error: Cannot save to cities.txt. Check permissions.\n";
        }
    }

    // Saves roads to data/roads.txt
    void save_roads_to_file() {
        ensure_data_directory();

        // Structure to hold road data
        struct RoadEntry {
            int nbr;
            string city1, city2;
            double budget;
        };
        vector<RoadEntry> existing_roads;

        // Read existing roads from file
        ifstream in_file("data/roads.txt");
        if (in_file.is_open()) {
            string line;
            getline(in_file, line); // Skip header
            while (getline(in_file, line)) {
                size_t first_tab = line.find('\t');
                size_t second_tab = line.find('\t', first_tab + 1);
                if (first_tab == string::npos || second_tab == string::npos) continue;

                int nbr;
                try {
                    nbr = stoi(line.substr(0, first_tab));
                } catch (...) {
                    continue;
                }

                string road = line.substr(first_tab + 1, second_tab - first_tab - 1);
                double budget;
                try {
                    budget = stod(line.substr(second_tab + 1));
                } catch (...) {
                    continue;
                }

                size_t dash_pos = road.find(" - ");
                if (dash_pos == string::npos) continue;
                string city1 = road.substr(0, dash_pos);
                string city2 = road.substr(dash_pos + 3);

                existing_roads.push_back({nbr, city1, city2, budget});
            }
            in_file.close();
        }

        // Update or add roads based on current state
        vector<RoadEntry> updated_roads;
        bool found;
        for (size_t i = 0; i < roads.size(); i++) {
            for (size_t j = i + 1; j < roads[i].size(); j++) {
                if (roads[i][j] == 1) {
                    string city1 = city_names[i];
                    string city2 = city_names[j];
                    double budget = budgets[i][j];
                    found = false;

                    // Check if road exists in file
                    for (auto& entry : existing_roads) {
                        if ((entry.city1 == city1 && entry.city2 == city2) ||
                            (entry.city1 == city2 && entry.city2 == city1)) {
                            updated_roads.push_back({entry.nbr, city1, city2, budget});
                            found = true;
                            break;
                        }
                    }

                    // New road: assign next Nbr
                    if (!found) {
                        int next_nbr = existing_roads.empty() ? 1 : existing_roads.back().nbr + 1;
                        updated_roads.push_back({next_nbr, city1, city2, budget});
                    }
                }
            }
        }
        // Sort roads by Nbr
        sort(updated_roads.begin(), updated_roads.end(),
             [](const RoadEntry& a, const RoadEntry& b) {
                 return a.nbr < b.nbr;
             });

        // Write sorted roads to file
        ofstream out_file("data/roads.txt");
        if (out_file.is_open()) {
            out_file << "Nbr\tRoad\t\t\tBudget\n";
            for (const auto& entry : updated_roads) {
                out_file << entry.nbr << "\t" << entry.city1 << " - " << entry.city2
                         << "\t" << fixed << setprecision(1) << entry.budget << endl;
            }
            out_file.close();
        } else {
            cout << "Error: Cannot save to roads.txt.\n";
        }
    }

    // Loads cities from data/cities.txt
    void load_cities_from_file() {
        ifstream cities_file("data/cities.txt");
        if (!cities_file.is_open()) return;

        string line;
        getline(cities_file, line); // Skip header
        while (getline(cities_file, line)) {
            size_t tab_pos = line.find('\t');
            if (tab_pos == string::npos) continue;
            string city_name = line.substr(tab_pos + 1);
            if (is_valid_city_name(city_name) && !city_exists(city_name)) {
                city_names.push_back(city_name);
                size_t n = city_names.size();
                for (auto& row : roads) row.resize(n, 0);
                for (auto& row : budgets) row.resize(n, 0.0);
                roads.push_back(vector<int>(n, 0));
                budgets.push_back(vector<double>(n, 0.0));
            }
        }
        cities_file.close();
    }

    // Loads roads and budgets from data/roads.txt
    void load_roads_from_file() {
        ifstream roads_file("data/roads.txt");
        if (!roads_file.is_open()) return;

        string line;
        getline(roads_file, line); // Skip header
        while (getline(roads_file, line)) {
            size_t first_tab = line.find('\t');
            size_t second_tab = line.find('\t', first_tab + 1);
            if (first_tab == string::npos || second_tab == string::npos) continue;

            string road = line.substr(first_tab + 1, second_tab - first_tab - 1);
            double budget;
            try {
                budget = stod(line.substr(second_tab + 1));
            } catch (...) {
                continue;
            }

            size_t dash_pos = road.find(" - ");
            if (dash_pos == string::npos) continue;
            string city1 = road.substr(0, dash_pos);
            string city2 = road.substr(dash_pos + 3);

            int i = get_city_index(city1);
            int j = get_city_index(city2);
            if (i != -1 && j != -1 && is_valid_budget(budget)) {
                roads[i][j] = roads[j][i] = 1;
                budgets[i][j] = budgets[j][i] = budget;
            }
        }
        roads_file.close();
    }

public:
    // Constructor initializes and loads data
    InfrastructureManager() {
        roads = vector<vector<int>>(0);
        budgets = vector<vector<double>>(0);
        load_cities_from_file();
        load_roads_from_file();
    }

    // Adding new cities
    void add_cities() {
        int k;
        while (true) {
            cout << "Enter the number of cities to add: ";
            if (cin >> k && is_valid_city_count(k)) {
                break;
            }
            cout << "Error: Enter a number between 1 and " << (MAX_CITIES - city_names.size()) << ".\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();

        for (int i = 0; i < k; i++) {
            string name;
            while (true) {
                cout << "Enter name for city " << (city_names.size() + 1) << ": ";
                getline(cin, name);
                if (name.empty()) {
                    cout << "Error: City name cannot be empty.\n";
                } else if (city_exists(name)) {
                    cout << "Error: City '" << name << "' already exists.\n";
                } else if (!is_valid_city_name(name)) {
                    cout << "Error: City name must be 2+ characters, contain at least one letter, "
                         << "and only include alphanumeric, space, or hyphen.\n";
                } else {
                    break;
                }
            }
            city_names.push_back(name);
            size_t n = city_names.size();
            for (auto& row : roads) row.resize(n, 0);
            for (auto& row : budgets) row.resize(n, 0.0);
            roads.push_back(vector<int>(n, 0));
            budgets.push_back(vector<double>(n, 0.0));
        }
        cout << k << " cities added successfully.\n";
        save_cities_to_file();
    }

    // Add road between cities
    void add_road() {
        string city1, city2;
        while (true) {
            cout << "Enter the name of the first city: ";
            getline(cin, city1);
            if (city_exists(city1)) {
                break;
            }
            cout << "Error: City '" << city1 << "' does not exist.\n";
        }
        while (true) {
            cout << "Enter the name of the second city: ";
            getline(cin, city2);
            if (city2 == city1) {
                cout << "Error: Cannot add a road from a city to itself.\n";
            } else if (!city_exists(city2)) {
                cout << "Error: City '" << city2 << "' does not exist.\n";
            } else if (roads[get_city_index(city1)][get_city_index(city2)] == 1) {
                cout << "Error: Road already exists between " << city1 << " and " << city2 << ".\n";
            } else {
                break;
            }
        }
        int i = get_city_index(city1);
        int j = get_city_index(city2);
        roads[i][j] = roads[j][i] = 1;
        cout << "Road added between " << city1 << " and " << city2 << ".\n";
        save_roads_to_file();
    }

    // Add budget for a road
    void add_budget() {
        string city1, city2;
        while (true) {
            cout << "Enter the name of the first city: ";
            getline(cin, city1);
            if (city_exists(city1)) break;
            cout << "Error: City '" << city1 << "' does not exist.\n";
        }
        while (true) {
            cout << "Enter the name of the second city: ";
            getline(cin, city2);
            if (!city_exists(city2)) {
                cout << "Error: City '" << city2 << "' does not exist.\n";
            } else if (roads[get_city_index(city1)][get_city_index(city2)] == 0) {
                cout << "Error: No road exists between " << city1 << " and " << city2 << ".\n";
            } else {
                break;
            }
        }
        int i = get_city_index(city1);
        int j = get_city_index(city2);
        double budget;
        while (true) {
            cout << "Enter the budget for the road: ";
            if (cin >> budget && is_valid_budget(budget)) break;
            cout << "Error: Budget must be between 0 and 1000 billion RWF.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();
        budgets[i][j] = budgets[j][i] = budget;
        cout << "Budget added for the road between " << city1 << " and " << city2 << ".\n";
        save_roads_to_file();
    }

    // Edit city name function
    void edit_city() {
        int index;
        while (true) {
            cout << "Enter the index of the city to be edited: ";
            if (cin >> index && is_valid_index(index)) break;
            cout << "Error: Invalid index. Enter a number between 1 and " << city_names.size() << ".\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();
        index--;
        string new_name;
        while (true) {
            cout << "Enter the new name of the city: ";
            getline(cin, new_name);
            if (new_name.empty()) {
                cout << "Error: City name cannot be empty.\n";
            } else if (city_exists(new_name)) {
                cout << "Error: City '" << new_name << "' already exists.\n";
            } else if (!is_valid_city_name(new_name)) {
                cout << "Error: City name must be 2+ characters, contain at least one letter, "
                     << "and only include alphanumeric, space, or hyphen.\n";
            } else {
                break;
            }
        }
        city_names[index] = new_name;
        cout << "City edited successfully.\n";
        save_cities_to_file();
        save_roads_to_file();
    }

    // Search city by index function
    void search_city() {
        int index;
        while (true) {
            cout << "Enter the index of the city: ";
            if (cin >> index && is_valid_index(index)) break;
            cout << "Error: Invalid index. Enter a number between 1 and " << city_names.size() << ".\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();
        index--;
        cout << "City at index " << (index + 1) << ": " << city_names[index] << "\n";
    }

    // Display cities function
    void display_cities() {
        if (city_names.empty()) {
            cout << "No cities recorded.\n";
            return;
        }
        cout << "Cities:\n";
        for (size_t i = 0; i < city_names.size(); i++) {
            cout << (i + 1) << ": " << city_names[i] << "\n";
        }
    }

    // Display roads function
    void display_roads() {
        if (roads.empty()) {
            cout << "No roads recorded.\n";
            return;
        }
        display_cities();
        cout << "\nRoads Adjacency Matrix:\n";
        for (const auto& row : roads) {
            for (int val : row) cout << val << " ";
            cout << "\n";
        }
    }

    // Display recorded data function
    void display_recorded_data() {
        if (city_names.empty()) {
            cout << "No data recorded.\n";
            return;
        }
        display_cities();
        cout << "\nRoads Adjacency Matrix:\n";
        for (const auto& row : roads) {
            for (int val : row) cout << val << " ";
            cout << "\n";
        }
        cout << "\nBudgets Adjacency Matrix:\n";
        for (const auto& row : budgets) {
            for (double val : row) cout << fixed << setprecision(1) << val << " ";
            cout << "\n";
        }
    }
};

// Displaying the menu
void display_menu() {
    cout << "\nMenu:\n"
         << "1. Add new city(ies)\n"
         << "2. Add roads between cities\n"
         << "3. Add the budget for roads\n"
         << "4. Edit city\n"
         << "5. Search for a city using its index\n"
         << "6. Display cities\n"
         << "7. Display roads\n"
         << "8. Display recorded data on console\n"
         << "9. Exit\n"
         << "Enter your choice: ";
}

// Main function
int main() {
    InfrastructureManager manager;
    cout << "\nWelcome to Rwanda Infrastructure Management System\n"
         << "---------------------------------------------------\n"
         << "Ministry of Infrastructure\n\n";

    int choice;
    do {
        display_menu();
        if (!(cin >> choice)) {
            cout << "Error: Please enter a number between 1 and 9.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        switch (choice) {
            case 1:
                // Add new cities
                manager.add_cities();
                break;
            case 2:
                // Add roads between cities
                manager.add_road();
                break;
            case 3:
                // Add budget for roads
                manager.add_budget();
                break;
            case 4:
                // Edit city name
                manager.edit_city();
                break;
            case 5:
                // Search for city by index
                manager.search_city();
                break;
            case 6:
                // Display cities
                manager.display_cities();
                break;
            case 7:
                // Display roads
                manager.display_roads();
                break;
            case 8:
                // Display recorded data
                manager.display_recorded_data();
                break;
            case 9:
                // Exit the program
                cout << "Exiting...\n";
                break;
            default:
                cout << "Error: Invalid choice. Enter a number between 1 and 9.\n";
        }
    } while (choice != 9);
    return 0;
}
