#pragma once
#include <algorithm>
#include <cmath>
#include <random>

namespace updated {
constexpr float width=1000, height=640, paddleHeight=96, radius=9;
enum class Phase { Menu, Serve, Playing, Paused, MatchOver };
struct Input { float left=0, right=0; };
struct Game {
    float leftY=320, rightY=320, x=500, y=320, vx=0, vy=0;
    float countdown=1.1f, aiTarget=320, aiClock=0;
    int leftScore=0,rightScore=0,leftRounds=0,rightRounds=0;
    int rally=0,bestRally=0,hits=0, difficulty=1, serveDirection=1;
    bool solo=true;
    Phase phase=Phase::Menu, resumePhase=Phase::Playing;
    std::mt19937 random{42};
    void start(bool single, int level) {
        *this=Game{}; solo=single; difficulty=std::clamp(level,0,2); phase=Phase::Serve;
    }
    void pause() {
        if(phase==Phase::Paused) phase=resumePhase;
        else if(phase==Phase::Playing || phase==Phase::Serve) {resumePhase=phase;phase=Phase::Paused;}
    }
    void serve(int direction) {
        x=width/2; y=height/2; vx=vy=0; rally=0;
        countdown=1.1f; serveDirection=direction; phase=Phase::Serve;
    }
    void point(bool left) {
        (left?leftScore:rightScore)++;
        if(leftScore==5 || rightScore==5) {
            (leftScore==5?leftRounds:rightRounds)++;
            leftScore=rightScore=0;
            if(leftRounds==2 || rightRounds==2) {phase=Phase::MatchOver;return;}
        }
        serve(left?1:-1);
    }
    void bounce(float paddleY, int direction) {
        float offset=std::clamp((y-paddleY)/(paddleHeight/2+radius),-1.f,1.f);
        float speed=std::min(780.f,std::hypot(vx,vy)+35.f);
        float angle=offset*1.02f;
        vx=direction*speed*std::cos(angle); vy=speed*std::sin(angle);
        ++hits; ++rally; bestRally=std::max(bestRally,rally);
    }
    void tick(float dt, Input input) {
        if(phase!=Phase::Playing && phase!=Phase::Serve) return;
        leftY=std::clamp(leftY+std::clamp(input.left,-1.f,1.f)*450*dt,48.f,592.f);
        float rightInput=std::clamp(input.right,-1.f,1.f);
        float rightSpeed=450;
        if(solo) {
            aiClock-=dt;
            if(aiClock<=0) {
                aiClock= difficulty==0?.24f:difficulty==1?.15f:.08f;
                std::uniform_real_distribution<float> error(-1,1);
                aiTarget=(vx>0?y:320)+error(random)*(difficulty==0?65:difficulty==1?30:12);
            }
            float delta=aiTarget-rightY;
            rightSpeed=difficulty==0?230:difficulty==1?330:420;
            rightInput=std::abs(delta)<8?0:std::clamp(delta/(rightSpeed*dt),-1.f,1.f);
        }
        rightY=std::clamp(rightY+rightInput*rightSpeed*dt,48.f,592.f);
        if(phase==Phase::Serve) {
            countdown-=dt;
            if(countdown<=0) {
                std::uniform_real_distribution<float> angle(-.45f,.45f);
                float a=angle(random);vx=serveDirection*340*std::cos(a);vy=340*std::sin(a);
                phase=Phase::Playing;
            }
            return;
        }
        // Swept face crossings avoid tunnelling through a paddle at high speed.
        float oldX=x,oldY=y;
        float nextX=x+vx*dt,nextY=y+vy*dt;
        auto collide=[&](float face,float paddle,int direction) {
            float t=(face-oldX)/(nextX-oldX);
            float impactY=oldY+(nextY-oldY)*t;
            if(std::abs(impactY-paddle)<=paddleHeight/2+radius) {
                y=impactY; bounce(paddle,direction);
                nextX=face+vx*dt*(1-t); nextY=impactY+vy*dt*(1-t);
            }
        };
        if(vx<0 && oldX>=65 && nextX<=65) collide(65,leftY,1);
        else if(vx>0 && oldX<=935 && nextX>=935) collide(935,rightY,-1);
        x=nextX;y=nextY;
        if(y<radius) {y=2*radius-y;vy=std::abs(vy);}
        if(y>height-radius) {y=2*(height-radius)-y;vy=-std::abs(vy);}
        if(x<-radius)point(false);
        else if(x>width+radius)point(true);
    }
};
}
