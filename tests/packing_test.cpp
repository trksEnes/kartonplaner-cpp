#include "kartonplaner/packing.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace karton;
int checks=0;
void check(bool ok,const char* label) {++checks;if(!ok)throw std::runtime_error(label);}
Settings tight() {Settings s;s.article_padding_mm=0;s.wall_margin_mm=0;s.budget_ms_per_carton=1000;return s;}
Candidate first(const std::vector<Article>& a,Carton c,Settings s=tight()) {auto r=solve(a,{c},s);check(r.error.empty(),"valid order");return r.candidates.at(0);}
int main() {
    try {
        check(rotations({10,20,30}).size()==6,"six rotations");
        check(rotations({10,10,30}).size()==3,"duplicate rotations removed");
        check(rotations({10,10,10}).size()==1,"cube has one rotation");
        auto r=first({{"A",{10,30,20},100,1}},{"A",{30,20,10},100});
        check(r.status==Status::Fits,"exact fit with rotation");
        check(validate_plan({{"A",{10,30,20},100,1}},r,tight()).empty(),"validate rotation plan");
        auto too_heavy=first({{"A",{10,10,10},101,1}},{"A",{100,100,100},100});
        check(too_heavy.status==Status::Weight,"weight boundary");
        auto volume=first({{"A",{10,10,10},1,9}},{"A",{20,20,20},100});
        check(volume.status==Status::Volume,"volume bound");
        auto long_one=first({{"A",{101,2,2},1,1}},{"A",{100,100,100},100});
        check(long_one.status==Status::Dimensions,"low volume but too long");
        auto geometric=first({{"A",{60,60,60},10,2}},{"box",{100,100,100},100});
        check(geometric.status==Status::NoArrangement,"enough volume alone does not prove fit");
        std::vector<Article> cubes{{"A",{10,10,10},10,8}};
        auto stack=first(cubes,{"cube",{20,20,20},80});
        check(stack.status==Status::Fits,"eight cubes in exact carton");
        check(validate_plan(cubes,stack,tight()).empty(),"cube packing geometry");
        for(std::size_t i=0;i<stack.placements.size();++i)for(std::size_t j=i+1;j<stack.placements.size();++j) {
            const auto& a=stack.placements[i];const auto& b=stack.placements[j];
            bool separate=a.x+a.occupied.length<=b.x||b.x+b.occupied.length<=a.x||
                a.y+a.occupied.width<=b.y||b.y+b.occupied.width<=a.y||
                a.z+a.occupied.height<=b.z||b.z+b.occupied.height<=a.z;
            check(separate,"independent non-overlap check");
        }
        auto corrupt=stack;corrupt.placements[1]=corrupt.placements[0];
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject duplicate copy");
        corrupt=stack;corrupt.placements[0].x=-1;
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject outside placement");
        corrupt=stack;corrupt.placements[1].x=corrupt.placements[0].x;
        corrupt.placements[1].y=corrupt.placements[0].y;corrupt.placements[1].z=corrupt.placements[0].z;
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject collision with distinct copy IDs");
        corrupt=stack;corrupt.placements[0].rotated.length+=1;
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject wrong rotation dimensions");
        corrupt=stack;corrupt.placements[0].occupied.length+=1;
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject inconsistent padding");
        corrupt=stack;std::reverse(corrupt.placements.begin(),corrupt.placements.end());
        check(!validate_plan(cubes,corrupt,tight()).empty(),"reject insertion before supporting items");
        auto floating=first({{"A",{10,10,10},1,1}},{"box",{30,30,30},100});
        floating.placements[0].z=10;
        check(!validate_plan({{"A",{10,10,10},1,1}},floating,tight()).empty(),"reject floating item");
        auto with_padding=tight();with_padding.article_padding_mm=1;
        check(first({{"A",{10,10,10},1,1}},{"box",{10,10,10},10},with_padding).status==Status::Volume,"padding changes fit");
        auto with_wall=tight();with_wall.wall_margin_mm=1;
        check(first({{"A",{10,10,10},1,1}},{"box",{12,12,12},10},with_wall).status==Status::Fits,"exact wall reserve");
        check(!solve({},example_cartons(),tight()).error.empty(),"reject empty order");
        check(!solve({{"A",{0,1,1},1,1}},example_cartons(),tight()).error.empty(),"reject zero dimension");
        check(!solve({{"A",{1,1,1},1,-1}},example_cartons(),tight()).error.empty(),"reject negative quantity");
        check(!solve({{"A",{1,1,1},1,81}},example_cartons(),tight()).error.empty(),"reject excessive quantity");
        check(!solve({{"A",{1,1,1},-1,1}},example_cartons(),tight()).error.empty(),"reject negative weight");
        check(!solve({{"A",{1,1,1},1,1}},{{"same",{2,2,2},2},{"same",{3,3,3},2}},tight()).error.empty(),"reject ambiguous carton name");
        auto examples=solve(example_order(),example_cartons(),Settings{});
        check(examples.error.empty()&&examples.recommended_index()>=0,"demo has valid recommendation");
        for(const auto& c:examples.candidates)if(c.status==Status::Fits)
            check(validate_plan(example_order(),c,Settings{}).empty(),"validate every demo option");
        auto idx=examples.recommended_index();
        check(std::is_sorted(examples.candidates.begin(),examples.candidates.end(),[](const auto& a,const auto& b){return a.carton.inside.volume()<b.carton.inside.volume();}),"cartons sorted by volume");
        std::cout<<"Demo: "<<examples.candidates[idx].carton.name<<", "<<examples.article_count<<" Artikel, "<<examples.elapsed_ms<<" ms\n";
        std::mt19937 rng(84931);
        for(int i=0;i<35;++i) {
            std::vector<Article> a;
            for(int j=0;j<3;++j)a.push_back({"A"+std::to_string(j),{int(rng()%25+5),int(rng()%25+5),int(rng()%25+5)},10,int(rng()%3+1)});
            auto test=solve(a,{{"one",{90,90,90},1000}},tight());
            check(test.recommended_index()>=0,"small randomized order has a plan");
            check(validate_plan(a,test.candidates[0],tight()).empty(),"random plan invariant");
        }
        std::atomic_bool cancel{true};
        check(solve(example_order(),example_cartons(),Settings{},&cancel).cancelled,"cancellation");
        std::vector<Article> large;
        for(int i=0;i<8;++i)large.push_back({"Position "+std::to_string(i),{30+i*3,40+i*2,20+i},50,10});
        auto large_result=solve(large,example_cartons(),Settings{});
        check(large_result.article_count==80&&large_result.recommended_index()>=0,"80 mixed units supported");
        for(const auto& c:large_result.candidates)if(c.status==Status::Fits)
            check(validate_plan(large,c,Settings{}).empty(),"validate larger order");
        std::cout<<checks<<" Prüfungen bestanden.\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FEHLER: "<<e.what()<<'\n';return 1;}
}
