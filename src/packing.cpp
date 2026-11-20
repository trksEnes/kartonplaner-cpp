#include "kartonplaner/packing.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <tuple>

namespace karton {
namespace {
using Clock = std::chrono::steady_clock;
struct Rect { int x, y, z, l, w, h; };
struct Unit { int article, copy; Size size; };

bool valid_size(Size s) {
    return s.length > 0 && s.width > 0 && s.height > 0 &&
           s.length <= 5000 && s.width <= 5000 && s.height <= 5000;
}
Rect rect(const Placement& p) { return {p.x,p.y,p.z,p.occupied.length,p.occupied.width,p.occupied.height}; }
bool overlap(const Rect& a, const Rect& b) {
    return a.x < b.x+b.l && b.x < a.x+a.l &&
           a.y < b.y+b.w && b.y < a.y+a.w &&
           a.z < b.z+b.h && b.z < a.z+a.h;
}
bool contained(const Rect& a, const Rect& b) {
    return a.x >= b.x && a.y >= b.y && a.z >= b.z &&
           a.x+a.l <= b.x+b.l && a.y+a.w <= b.y+b.w && a.z+a.h <= b.z+b.h;
}
std::int64_t volume(const Rect& r) { return std::int64_t(r.l)*r.w*r.h; }

bool supported(const Rect& r, const std::vector<Placement>& placed, int floor) {
    if (r.z == floor) return true;
    std::int64_t support = 0;
    // Non-overlapping boxes at the same level have non-overlapping top interiors.
    // Summing their clipped areas therefore measures the union without double counting.
    for (const auto& p : placed) {
        Rect b = rect(p);
        if (b.z+b.h != r.z) continue;
        int l = std::max(0,std::min(r.x+r.l,b.x+b.l)-std::max(r.x,b.x));
        int w = std::max(0,std::min(r.y+r.w,b.y+b.w)-std::max(r.y,b.y));
        support += std::int64_t(l)*w;
    }
    return support == std::int64_t(r.l)*r.w;
}

std::vector<Rect> subtract(const std::vector<Rect>& spaces, const Rect& used) {
    std::vector<Rect> next;
    auto add = [&](Rect r) { if (r.l>0 && r.w>0 && r.h>0) next.push_back(r); };
    for (auto s : spaces) {
        if (!overlap(s,used)) { add(s); continue; }
        // Split every intersecting free cuboid; free cuboids may overlap one another.
        // Only contained duplicates are removed, so a future item can span a split.
        if (used.x>s.x) add({s.x,s.y,s.z,used.x-s.x,s.w,s.h});
        if (used.x+used.l<s.x+s.l) add({used.x+used.l,s.y,s.z,s.x+s.l-used.x-used.l,s.w,s.h});
        if (used.y>s.y) add({s.x,s.y,s.z,s.l,used.y-s.y,s.h});
        if (used.y+used.w<s.y+s.w) add({s.x,used.y+used.w,s.z,s.l,s.y+s.w-used.y-used.w,s.h});
        if (used.z>s.z) add({s.x,s.y,s.z,s.l,s.w,used.z-s.z});
        if (used.z+used.h<s.z+s.h) add({s.x,s.y,used.z+used.h,s.l,s.w,s.z+s.h-used.z-used.h});
    }
    std::sort(next.begin(),next.end(),[](const Rect& a,const Rect& b) {
        return std::tie(a.x,a.y,a.z,a.l,a.w,a.h)<std::tie(b.x,b.y,b.z,b.l,b.w,b.h);
    });
    next.erase(std::unique(next.begin(),next.end(),[](const Rect& a,const Rect& b) {
        return std::tie(a.x,a.y,a.z,a.l,a.w,a.h)==std::tie(b.x,b.y,b.z,b.l,b.w,b.h);
    }),next.end());
    std::vector<Rect> pruned;
    for (std::size_t i=0;i<next.size();++i) {
        bool inside = false;
        for (std::size_t j=0;j<next.size();++j)
            if (i!=j && contained(next[i],next[j])) { inside=true; break; }
        if (!inside) pruned.push_back(next[i]);
    }
    return pruned;
}

bool stopped(const std::atomic_bool* cancel) { return cancel && cancel->load(std::memory_order_relaxed); }

std::vector<Unit> ordered_units(const std::vector<Article>& articles, int attempt) {
    std::vector<Unit> units;
    for (std::size_t a=0;a<articles.size();++a)
        for (int q=1;q<=articles[a].quantity;++q) units.push_back({int(a),q,articles[a].size});
    int mode = (attempt/4)%6;
    std::mt19937 rng(43271u+std::uint32_t(attempt/24)*7919u);
    std::vector<unsigned> noise(units.size()); for(auto& n:noise)n=rng();
    std::vector<int> indices(units.size()); std::iota(indices.begin(),indices.end(),0);
    auto key = [&](int i) {
        auto s=units[i].size; auto sides=std::array<int,3>{s.length,s.width,s.height};
        std::sort(sides.begin(),sides.end());
        if (mode==0) return double(s.volume());
        if (mode==1) return double(sides[2])*1e9+double(s.volume());
        if (mode==2) return double(sides[1])*sides[2]*1e5+sides[0];
        if (mode==3) return double(sides[0])*1e9+double(s.volume());
        if (mode==4) return double(s.volume())*(0.70+double(noise[i]%600)/1000.0);
        return double(noise[i]);
    };
    std::stable_sort(indices.begin(),indices.end(),[&](int a,int b){return key(a)>key(b);});
    std::vector<Unit> result; for(int i:indices)result.push_back(units[i]);
    return result;
}

std::vector<Placement> try_pack(const std::vector<Unit>& units, const Carton& carton,
                              const Settings& settings, int policy, Clock::time_point deadline,
                              const std::atomic_bool* cancel) {
    int margin=settings.wall_margin_mm, pad=settings.article_padding_mm;
    std::vector<Rect> spaces{{margin,margin,margin,carton.inside.length-2*margin,
                             carton.inside.width-2*margin,carton.inside.height-2*margin}};
    std::vector<Placement> placed;
    int envelope_l=0,envelope_w=0,envelope_h=0;
    for(const auto& u:units) {
        if(stopped(cancel)||Clock::now()>deadline)return {};
        bool found=false; Placement best; double best_score=std::numeric_limits<double>::infinity();
        for(const auto& space:spaces) {
            for(auto dims:rotations(u.size)) {
                Size outer{dims.length+2*pad,dims.width+2*pad,dims.height+2*pad};
                if(outer.length>space.l||outer.width>space.w||outer.height>space.h)continue;
                // Try each horizontal corner. High-z corners would intentionally float.
                for(int corner=0;corner<4;++corner) {
                    Rect r{space.x+((corner&1)?space.l-outer.length:0),
                           space.y+((corner&2)?space.w-outer.width:0),space.z,
                           outer.length,outer.width,outer.height};
                    bool collision=false;
                    for(const auto& p:placed)if(overlap(r,rect(p))){collision=true;break;}
                    if(collision||!supported(r,placed,margin))continue;
                    int l=std::max(envelope_l,r.x+r.l-margin),w=std::max(envelope_w,r.y+r.w-margin),
                        h=std::max(envelope_h,r.z+r.h-margin);
                    double box_volume=double(carton.inside.volume());
                    double compact=double(l)*w*h/box_volume;
                    double elevation=double(r.z-margin)/carton.inside.height;
                    double top=double(h)/carton.inside.height;
                    double slack=double(volume(space)-std::int64_t(r.l)*r.w*r.h)/box_volume;
                    double score;
                    if(policy==0)score=top*100+compact*12+elevation*2+slack*0.1;
                    else if(policy==1)score=compact*100+top*4+elevation*2+slack*0.01;
                    else if(policy==2)score=slack*100+compact*4+elevation*2+top;
                    else score=elevation*100+compact*15+top*2+slack*0.1;
                    score+=(double(r.x)/carton.inside.length+double(r.y)/carton.inside.width)*0.0001;
                    if(!found||score<best_score-1e-12) {
                        found=true;best_score=score;
                        best={u.article,u.copy,dims,outer,r.x,r.y,r.z};
                    }
                }
            }
        }
        if(!found)return {};
        placed.push_back(best); spaces=subtract(spaces,rect(best));
        envelope_l=std::max(envelope_l,best.x+best.occupied.length-margin);
        envelope_w=std::max(envelope_w,best.y+best.occupied.width-margin);
        envelope_h=std::max(envelope_h,best.z+best.occupied.height-margin);
    }
    return placed;
}
} // namespace

std::int64_t Size::volume() const {return std::int64_t(length)*width*height;}
bool Size::operator==(const Size& o)const{return length==o.length&&width==o.width&&height==o.height;}
int Result::recommended_index()const {
    for(std::size_t i=0;i<candidates.size();++i)if(candidates[i].status==Status::Fits)return int(i);
    return -1;
}
std::vector<Size> rotations(Size s) {
    std::array<int,3> a{s.length,s.width,s.height};std::sort(a.begin(),a.end());
    std::vector<Size> result;
    do {result.push_back({a[0],a[1],a[2]});}while(std::next_permutation(a.begin(),a.end()));
    return result;
}
std::vector<Carton> example_cartons() {
    return {{"XS",{200,150,100},2000},{"S",{250,200,150},5000},
            {"M",{300,250,200},8000},{"M+",{350,250,250},10000},
            {"L",{400,300,250},15000},{"L+",{450,350,300},20000},
            {"XL",{500,400,350},25000},{"XXL",{600,400,400},30000}};
}
std::vector<Article> example_order() {
    return {{"LED-Set",{120,80,60},350,4},{"Netzteil",{160,100,60},550,1},
            {"Zubehörbox",{140,90,40},180,2}};
}
std::string input_error(const std::vector<Article>& articles,const std::vector<Carton>& cartons,
                        const Settings& settings) {
    if(articles.empty())return "Füge zuerst mindestens einen Artikel hinzu.";
    if(cartons.empty())return "Es ist kein Karton hinterlegt.";
    if(articles.size()>80||cartons.size()>40)return "Zu viele Positionen oder Kartons.";
    int count=0;
    for(const auto& a:articles) {
        if(a.name.empty()||a.name.size()>240)return "Jeder Artikel braucht einen Namen mit höchstens 240 Zeichen.";
        if(!valid_size(a.size))return "Artikelmaße müssen zwischen 1 und 5000 mm liegen.";
        if(a.quantity<1||a.quantity>max_article_count)return "Ungültige Artikelmenge.";
        if(a.weight_g<0||a.weight_g>1000000)return "Artikelgewicht muss zwischen 0 und 1000 kg liegen.";
        count+=a.quantity;
    }
    if(count>max_article_count)return "Die Demo berechnet höchstens 80 Einzelartikel pro Auftrag.";
    std::set<std::string> names;
    for(const auto& c:cartons) {
        if(c.name.empty()||c.name.size()>120)return "Jeder Karton braucht einen Namen.";
        if(!names.insert(c.name).second)return "Kartonnamen müssen eindeutig sein.";
        if(!valid_size(c.inside))return "Kartoninnenmaße müssen zwischen 1 und 5000 mm liegen.";
        if(c.max_payload_g<=0||c.max_payload_g>1000000)return "Die maximale Zuladung muss zwischen 1 g und 1000 kg liegen.";
    }
    if(settings.article_padding_mm<0||settings.article_padding_mm>200||settings.wall_margin_mm<0||settings.wall_margin_mm>200)
        return "Polster und Randreserve müssen zwischen 0 und 20 cm liegen.";
    if(settings.attempts<1||settings.attempts>128||settings.budget_ms_per_carton<1||settings.budget_ms_per_carton>10000)
        return "Ungültige Suchbegrenzung.";
    return {};
}

Result solve(const std::vector<Article>& articles,const std::vector<Carton>& cartons,
             const Settings& settings,const std::atomic_bool* cancel) {
    auto start=Clock::now();Result result;
    result.error=input_error(articles,cartons,settings);if(!result.error.empty())return result;
    for(const auto& a:articles) {
        result.article_count+=a.quantity;result.weight_g+=std::int64_t(a.weight_g)*a.quantity;
        result.article_volume_mm3+=a.size.volume()*a.quantity;
        Size protected_size{a.size.length+2*settings.article_padding_mm,a.size.width+2*settings.article_padding_mm,
                            a.size.height+2*settings.article_padding_mm};
        result.protected_volume_mm3+=protected_size.volume()*a.quantity;
    }
    auto sorted=cartons;
    std::stable_sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.inside.volume()<b.inside.volume();});
    for(const auto& c:sorted) {
        if(stopped(cancel)){result.cancelled=true;break;}
        Candidate candidate;candidate.carton=c;
        Size available{c.inside.length-2*settings.wall_margin_mm,c.inside.width-2*settings.wall_margin_mm,
                       c.inside.height-2*settings.wall_margin_mm};
        if(result.weight_g>c.max_payload_g) {
            candidate.status=Status::Weight;candidate.explanation="Maximale Zuladung überschritten.";
        } else if(!valid_size(available)) {
            candidate.status=Status::Dimensions;candidate.explanation="Die Randreserve lässt keinen nutzbaren Innenraum.";
        } else if(result.protected_volume_mm3>available.volume()) {
            candidate.status=Status::Volume;candidate.explanation="Zu wenig nutzbares Volumen einschließlich Artikelpolster.";
        } else {
            bool can_fit=true;
            for(const auto& a:articles) {
                bool one=false;
                for(auto s:rotations(a.size))if(s.length+2*settings.article_padding_mm<=available.length&&
                    s.width+2*settings.article_padding_mm<=available.width&&s.height+2*settings.article_padding_mm<=available.height)one=true;
                if(!one){can_fit=false;break;}
            }
            if(!can_fit) {
                candidate.status=Status::Dimensions;candidate.explanation="Mindestens ein Artikel passt in keiner Drehung hinein.";
            } else {
                auto deadline=Clock::now()+std::chrono::milliseconds(settings.budget_ms_per_carton);
                for(int attempt=0;attempt<settings.attempts;++attempt) {
                    if(stopped(cancel)){result.cancelled=true;break;}
                    if(Clock::now()>deadline){candidate.time_limited=true;break;}
                    candidate.attempts_used++;
                    candidate.placements=try_pack(ordered_units(articles,attempt),c,settings,attempt%4,deadline,cancel);
                    if(candidate.placements.size()==std::size_t(result.article_count)) {
                        auto error=validate_plan(articles,candidate,settings);
                        if(error.empty()) {candidate.status=Status::Fits;candidate.explanation="Vollständiger Packplan gefunden.";break;}
                        candidate.placements.clear();
                    }
                }
                if(result.cancelled){candidate.status=Status::Cancelled;candidate.explanation="Suche abgebrochen.";}
                else if(candidate.status!=Status::Fits) {
                    candidate.placements.clear();candidate.status=Status::NoArrangement;
                    candidate.explanation=candidate.time_limited?"Im Zeitlimit keinen Packplan gefunden.":"Mit den getesteten Anordnungen keinen Packplan gefunden.";
                }
            }
        }
        result.candidates.push_back(std::move(candidate));
    }
    result.elapsed_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-start).count();
    return result;
}

std::string validate_plan(const std::vector<Article>& articles,const Candidate& candidate,const Settings& settings) {
    auto error=input_error(articles,{candidate.carton},settings);if(!error.empty())return error;
    int wanted=0;std::int64_t weight=0;
    for(const auto& a:articles){wanted+=a.quantity;weight+=std::int64_t(a.quantity)*a.weight_g;}
    if(candidate.placements.size()!=std::size_t(wanted))return "Nicht alle Artikel sind enthalten.";
    if(weight>candidate.carton.max_payload_g)return "Gewichtsgrenze überschritten.";
    std::set<std::pair<int,int>> copies;
    std::vector<Placement> placed;
    const int m=settings.wall_margin_mm,pad=settings.article_padding_mm;
    Rect usable{m,m,m,candidate.carton.inside.length-2*m,candidate.carton.inside.width-2*m,candidate.carton.inside.height-2*m};
    // Validate the exported insertion order as well as the final geometry.
    for(const auto& p:candidate.placements) {
        if(p.article_index<0||p.article_index>=int(articles.size()))return "Unbekannter Artikel im Packplan.";
        const auto& a=articles[std::size_t(p.article_index)];
        if(p.copy_number<1||p.copy_number>a.quantity||!copies.insert({p.article_index,p.copy_number}).second)
            return "Artikel doppelt oder mit ungültiger Stücknummer enthalten.";
        auto options=rotations(a.size);
        if(std::find(options.begin(),options.end(),p.rotated)==options.end())return "Ungültige Drehung.";
        if(!(p.occupied==Size{p.rotated.length+2*pad,p.rotated.width+2*pad,p.rotated.height+2*pad}))
            return "Artikelpolster wurde nicht eingehalten.";
        Rect r=rect(p);
        if(!contained(r,usable))return "Artikel außerhalb des nutzbaren Innenraums.";
        for(const auto& other:placed)if(overlap(r,rect(other)))return "Artikel überschneiden sich.";
        if(!supported(r,placed,m))return "Ein Artikel ist nicht vollständig unterstützt.";
        placed.push_back(p);
    }
    return {};
}
} // namespace karton
