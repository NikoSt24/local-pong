#include "game_updated.hpp"
#include <cstdlib>
#include <iostream>
using namespace updated;
int checks=0;
void require(bool condition,const char* message) {++checks;if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main() {
    Game g;g.start(false,1);g.tick(2,{-1,1});
    require(g.leftY==48 && g.rightY==592,"paddles clamp to court");
    require(g.phase==Phase::Playing,"serve transitions into play");
    g.pause();float x=g.x;g.tick(.1f,{});require(g.x==x,"paused simulation stays frozen");
    g.pause();require(g.phase==Phase::Playing,"pause restores phase");
    g.leftY=320;g.x=70;g.y=340;g.vx=-780;g.vy=0;g.tick(1.f/120,{});
    require(g.vx>0 && g.hits==1,"swept collision catches fast ball");
    require(g.vy>0 && std::hypot(g.vx,g.vy)<=780.01f,"impact angle and total speed cap");
    g.x=500;g.y=10;g.vx=100;g.vy=-400;g.tick(1.f/120,{});
    require(g.y>=radius && g.vy>0,"wall rebound remains in bounds");
    g.start(false,1);for(int i=0;i<10;i++)g.point(true);
    require(g.phase==Phase::MatchOver && g.leftRounds==2,"two round win ends match");
    g.start(false,1);for(int i=0;i<5;i++)g.point(true);for(int i=0;i<10;i++)g.point(false);
    require(g.phase==Phase::MatchOver && g.rightRounds==2 && g.leftRounds==1,"three round match scores correctly");
    g.start(false,1);require(g.hits==0 && g.leftRounds==0 && g.phase==Phase::Serve,"restart clears match state");
    g.pause();g.pause();require(g.phase==Phase::Serve,"pause preserves serve countdown phase");
    g.phase=Phase::Playing;g.x=1010;g.vx=340;g.tick(1.f/120,{});
    require(g.leftScore==1 && g.phase==Phase::Serve,"goal resets ball for serve");
    Game a,b;a.start(true,2);b.start(true,2);
    for(int i=0;i<5000;i++){a.tick(1.f/120,{});b.tick(1.f/120,{});}
    require(a.x==b.x && a.y==b.y && a.rightScore==b.rightScore,"seeded simulation is repeatable");
    require(a.rightY>=48 && a.rightY<=592,"AI respects paddle bounds");
    std::cout<<checks<<" checks passed\n";
}
