#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace karton {

// Millimetres and grams keep every geometric and weight check exact.
struct Size {
    int length = 0, width = 0, height = 0;
    std::int64_t volume() const;
    bool operator==(const Size& other) const;
};

struct Article {
    std::string name;
    Size size;
    int weight_g = 0;
    int quantity = 1;
};

struct Carton {
    std::string name;
    Size inside;
    int max_payload_g = 0;
};

struct Settings {
    int article_padding_mm = 2; // Added on all six sides of each article.
    int wall_margin_mm = 5;    // Kept free along each carton wall.
    int attempts = 32;
    int budget_ms_per_carton = 350;
};

struct Placement {
    int article_index = 0;
    int copy_number = 1;
    Size rotated; // Article itself, excluding padding.
    Size occupied;
    int x = 0, y = 0, z = 0; // Outer padding corner, relative to carton interior.
};

enum class Status { Fits, Weight, Volume, Dimensions, NoArrangement, Cancelled };

struct Candidate {
    Carton carton;
    Status status = Status::NoArrangement;
    std::string explanation;
    std::vector<Placement> placements;
    int attempts_used = 0;
    bool time_limited = false;
};

struct Result {
    std::vector<Candidate> candidates; // Ascending interior volume.
    std::int64_t article_volume_mm3 = 0;
    std::int64_t protected_volume_mm3 = 0;
    std::int64_t weight_g = 0;
    int article_count = 0;
    long long elapsed_ms = 0;
    bool cancelled = false;
    std::string error;
    int recommended_index() const;
};

constexpr int max_article_count = 80;
std::vector<Carton> example_cartons();
std::vector<Article> example_order();
std::vector<Size> rotations(Size size);
std::string input_error(const std::vector<Article>& articles,
                        const std::vector<Carton>& cartons, const Settings& settings);

// Multiple deterministic orders and placement policies search free cuboids.
// A failed heuristic search is deliberately distinct from a proved bound.
Result solve(const std::vector<Article>& articles, const std::vector<Carton>& cartons,
             const Settings& settings, const std::atomic_bool* cancel = nullptr);

// Re-check a proposed plan against the original order, independently of free-space state.
std::string validate_plan(const std::vector<Article>& articles, const Candidate& candidate,
                          const Settings& settings);

} // namespace karton
