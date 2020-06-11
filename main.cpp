#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <SFML/Graphics.hpp>

#include "imgui.h"
#include "imgui_SFML.h"

using namespace std;

vector<sf::Vertex> init_grid(int window_size, int row_cells_count) {
    vector<sf::Vertex> grid_lines;
    float line_spacing = (float)window_size / (float)row_cells_count;

    // add vertical lines
    for (int i = 1; i < row_cells_count; ++i) {
        for (int j = 0; j < row_cells_count; ++j) {
            sf::Vertex v;
            v.color = sf::Color::Black;
            v.position = { i * line_spacing, 0 };
            grid_lines.push_back(v);
            
            v.position = { i * line_spacing, (float)window_size };
            grid_lines.push_back(v);
        }
    }

    // add horizontal lines
    for (int i = 1; i < row_cells_count; ++i) {
        for (int j = 0; j < row_cells_count; ++j) {
            sf::Vertex v;
            v.color = sf::Color::Black;
            v.position = { 0, i * line_spacing};
            grid_lines.push_back(v);

            v.position = { (float)window_size, i * line_spacing};
            grid_lines.push_back(v);
        }
    }
    return grid_lines;
}

void perform_hat_rule(vector<vector<int>>& cells) {
    auto cached_cells = cells;
    for (int i = 0; i < cached_cells.size(); ++i) {
        for (int j = 0; j < cached_cells.size(); ++j) {
            int hat_count = 0;
            if (j > 0) {
                hat_count += cached_cells[i][j - 1];
            }
            if (j < cells.size() - 1) {
                hat_count += cached_cells[i][j + 1];
            }
            if (hat_count == 1) {
                cells[i][j] = 1;
            }
            else {
                cells[i][j] = 0;
            }
            
        }
    }
}
void perform_game_of_life(vector<vector<int>>& cells) {
    auto cached_cells = cells;
    vector <vector<int>> n_inds = { {-1, -1}, {-1, 0}, {-1, +1}, {0, +1}, {+1, +1}, {+1, 0}, {+1, -1}, {0, -1} };
    for (int i = 0; i < cached_cells.size(); ++i) {
        for (int j = 0; j < cached_cells.size(); ++j) {
            int n_count = 0;
            for (auto inds : n_inds) {
                if (i + inds[0] >= 0 && i + inds[0] < cells.size() && j + inds[1] >=0 && j + inds[1] < cells.size()) {
                    n_count += cached_cells[i + inds[0]][j + inds[1]];
                }
            }
            if (n_count != 2 && n_count != 3) {
                cells[i][j] = 0;
            } else if(n_count == 3) {
                cells[i][j] = 1;
            }
        }
    }
}

void initialize_map(mt19937& mt, uniform_int_distribution<int>& uid, int fill_probability, vector<vector<int>>& cells) {
    int N = cells.size();

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == 0 || i == N - 1 || j == 0 || j == N - 1) {
                cells[i][j] = 1;
            }
            else {
                cells[i][j] = uid(mt) < fill_probability ? 1 : 0;
            }
        }
    }
}

// num_iterations - how many times to perform the algorithm
// layer_count - how many cells far it can have as neighbors
// layer_weights - used for weighing neighbors to decide the next state
// use_prev_states - whether to use data at (t-1) i.e cached the data and used it for further computations
// use_current_cell - use the current cell for weighting
// max > weight > min - the state stays the same
// weight <= min - dies
// weigth >= max - new one
void generate_map(vector<vector<int>>& cells,
    int num_iterations,
    int layer_count,
    const vector<float>& layer_weights,
    bool use_prev_states,
    bool use_current_cell,
    float current_cell_weigth,
    float min_weight_to_die, 
    float min_weight_to_spawn,
    bool prefer_walling) {
    
    vector<vector<int>> cached_cells;
    int N = cells.size();

    for (int it = 0; it < num_iterations; ++it) {

        if (use_prev_states) {
            cached_cells = cells;
        }    
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                float weight = 0;
              
                for (int k = i - layer_count; k <= i + layer_count; ++k) {
                    for (int l = j - layer_count; l <= j + layer_count; ++l) {
                        if (k >= 0 && k < N && l >= 0 && l < N) {
                            float cell_state = cells[k][l];
                            if (use_prev_states) {
                                cell_state = cached_cells[k][l];
                            }

                            if (k != i || l != j) {
                                weight += cell_state * layer_weights[max(abs(i - k), abs(j - l)) - 1];
                            }
                            else if (use_current_cell) {
                                weight += cell_state * current_cell_weigth;
                            }
                        }
                        else {
                            if (prefer_walling) {
                                ++weight;
                            }
                        }
                    }
                }
                
                if (weight < min_weight_to_die) {
                    cells[i][j] = 0;
                }
                else if (weight > min_weight_to_spawn) {
                    cells[i][j] = 1;
                }
            }
        }
    }
}

int main()
{
    int window_size = 800;
    sf::RenderWindow window(sf::VideoMode(window_size, window_size), "CellularAutomata");
    window.setFramerateLimit(75);
    ImGui::SFML::Init(window);

    int row_cells_count = 80;
    auto grid_lines = init_grid(window_size, row_cells_count);

    vector<vector<int>> cells(row_cells_count, vector<int>(row_cells_count, 0));
    int mid = row_cells_count / 2;
    cells[mid][mid] = 1;
    
    /*cells[mid + 1][mid - 1] = 1;
    cells[mid + 1][mid + 1] = 1;
    
    cells[mid + 2][mid - 2] = 1;
    cells[mid + 2][mid + 2] = 1;

    cells[mid + 3][mid - 3] = 1;
    cells[mid + 3][mid - 1] = 1;
    cells[mid + 3][mid + 1] = 1;
    cells[mid + 3][mid + 3] = 1;*/
    
    

    sf::RectangleShape cell_shape;
    cell_shape.setFillColor(sf::Color(0, 0, 255, 145));
    float cell_size = (float)window_size / (float)row_cells_count;
    
    cell_shape.setSize({ cell_size - 1, cell_size - 1});

    float step_time_interval = 0.5;
    float accum_time = 0;
    sf::Clock clock;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> uid(0, 99);
    int fill_probability = 50;
    int num_iterations = 0;
    int layer_count = 1;
    vector<float> layer_weights(1, 1);
    bool use_prev_states = false;
    bool use_current_cell = false;
    float current_cell_weigth = 0;
    float min_weight_to_die = 4;
    float min_weight_to_spawn = 4;
    bool prefer_walling = false;
    bool do_game_of_life = false;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float dt = clock.getElapsedTime().asMilliseconds() / 1000.0;

        ImGui::SFML::Update(window, clock.restart());
        ImGui::Begin("Edit Menu");

        ImGui::SliderInt("Fill probability", &fill_probability, 0, 100);
        ImGui::Spacing();

        ImGui::SliderInt("Num Iterations", &num_iterations, 0, 10);
        ImGui::SliderInt("Layer Count", &layer_count, 1, 5);
        if (layer_count != layer_weights.size()) {
            layer_weights = vector<float>(layer_count, 1);
        }
        for (int i = 0; i < layer_count; ++i) {
            ImGui::SliderFloat(string("LayerW" + to_string(i + 1)).c_str(), &layer_weights[i], 0, 1);
        }
        
        ImGui::Checkbox("use prev states", &use_prev_states);
        ImGui::Checkbox("use curr cell", &use_current_cell);
        ImGui::SliderFloat("Curr cell weight", &current_cell_weigth, 0, 1);
        ImGui::SliderFloat("min W to die", &min_weight_to_die, 0, 100);
        ImGui::SliderFloat("min W to spawn", &min_weight_to_spawn, 0, 100);
        ImGui::Checkbox("prefer walling", &prefer_walling);
        ImGui::Checkbox("perform Game of Life", &do_game_of_life);

        if (ImGui::Button("Generate")) {
            initialize_map(mt, uid, fill_probability, cells);
            generate_map(
                cells, num_iterations, 
                layer_count, 
                layer_weights, 
                use_prev_states, 
                use_current_cell, 
                current_cell_weigth, 
                min_weight_to_die, 
                min_weight_to_spawn,
                prefer_walling);
        }
        ImGui::End();

        if (do_game_of_life) {
            accum_time += dt;
            if (accum_time > step_time_interval) {
                perform_game_of_life(cells);
                //perform_hat_rule(cells);
                accum_time = 0;
            }
        }

        window.clear(sf::Color::White);
        
        window.draw(grid_lines.data(), grid_lines.size(), sf::Lines);
        for (int i = 0; i < cells.size(); ++i) {
            for (int j = 0; j < cells.size(); ++j) {
                if (cells[i][j] == 1) {
                    cell_shape.setPosition(cell_size * j + 0.5, cell_size * i + 0.5);
                    window.draw(cell_shape);
                }
            }
        }

        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}