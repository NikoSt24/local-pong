#include "game_updated.hpp"
#include <SFML/Graphics.hpp>
#include <deque>
#include <iostream>
#include <string>

int main() {
    sf::RenderWindow window(sf::VideoMode({1200,880}),"LOCAL PONG / Updated Edition");
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    sf::Font font;
    bool loaded=false;
    for(const auto* path:{"assets/fonts/DejaVuSans.ttf","/System/Library/Fonts/Supplemental/Arial.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf","C:/Windows/Fonts/arial.ttf"})
        if(font.openFromFile(path)) {loaded=true;break;}
    if(!loaded) {std::cerr<<"A font is required. See README.md.\n";return 1;}
    updated::Game game;
    bool solo=true;int difficulty=1;bool w=false,s=false,up=false,down=false;
    std::deque<sf::Vector2f> trail;
    sf::Clock clock;float accumulator=0;
    const sf::Color cyan(95,233,221), orange(255,172,103), muted(133,151,170);
    auto text=[&](std::string value,float x,float y,unsigned size,sf::Color color) {
        sf::Text label(font,value,size);label.setPosition({x,y});label.setFillColor(color);window.draw(label);
    };
    auto rect=[&](float x,float y,float width,float height,sf::Color color) {
        sf::RectangleShape r({width,height});r.setPosition({x,y});r.setFillColor(color);window.draw(r);
    };
    while(window.isOpen()) {
        while(auto event=window.pollEvent()) {
            if(event->is<sf::Event::Closed>())window.close();
            if(event->is<sf::Event::FocusLost>()) {
                w=s=up=down=false;
                if(game.phase==updated::Phase::Playing || game.phase==updated::Phase::Serve)game.pause();
            }
            if(auto key=event->getIf<sf::Event::KeyPressed>()) {
                auto k=key->code;
                if(k==sf::Keyboard::Key::W)w=true;
                if(k==sf::Keyboard::Key::S)s=true;
                if(k==sf::Keyboard::Key::Up)up=true;
                if(k==sf::Keyboard::Key::Down)down=true;
                if(k==sf::Keyboard::Key::Escape) {game.phase=updated::Phase::Menu;trail.clear();}
                if(k==sf::Keyboard::Key::P)game.pause();
                if(k==sf::Keyboard::Key::R && game.phase!=updated::Phase::Menu) {game.start(solo,difficulty);trail.clear();}
                if(game.phase==updated::Phase::Menu) {
                    if(k==sf::Keyboard::Key::Num1)solo=true;
                    if(k==sf::Keyboard::Key::Num2)solo=false;
                    if(k==sf::Keyboard::Key::D)difficulty=(difficulty+1)%3;
                    if(k==sf::Keyboard::Key::Enter) {game.start(solo,difficulty);trail.clear();}
                } else if(game.phase==updated::Phase::MatchOver && k==sf::Keyboard::Key::Enter) {game.start(solo,difficulty);trail.clear();}
            }
            if(auto key=event->getIf<sf::Event::KeyReleased>()) {
                if(key->code==sf::Keyboard::Key::W)w=false;
                if(key->code==sf::Keyboard::Key::S)s=false;
                if(key->code==sf::Keyboard::Key::Up)up=false;
                if(key->code==sf::Keyboard::Key::Down)down=false;
            }
        }
        accumulator+=std::min(clock.restart().asSeconds(),.1f);
        while(accumulator>=1.f/120) {
            game.tick(1.f/120,{float(s)-float(w),float(down)-float(up)});
            if(game.phase==updated::Phase::Playing) {
                trail.push_front({game.x,game.y});if(trail.size()>18)trail.pop_back();
            } else if(game.phase==updated::Phase::Serve)trail.clear();
            accumulator-=1.f/120;
        }
        auto size=window.getSize();
        if(size.x==0 || size.y==0)continue;
        sf::View view(sf::FloatRect({0,0},{1000,760}));
        float ratio=(float(size.x)/size.y)/(1000.f/760);
        if(ratio>1)view.setViewport(sf::FloatRect({(1-1/ratio)/2,0},{1/ratio,1}));
        else view.setViewport(sf::FloatRect({0,(1-ratio)/2},{1,ratio}));
        window.setView(view);window.clear(sf::Color(9,15,25));
        text("LOCAL / PONG",32,14,22,cyan);
        text("UPDATED EDITION   /   C++ + SFML",600,20,14,muted);
        rect(20,65,960,640,sf::Color(16,26,40));
        // Court coordinates are translated down into the HUD frame.
        for(int i=0;i<20;i++)rect(498,65+i*32,3,14,sf::Color(39,54,70));
        text(std::to_string(game.leftScore),400,78,48,cyan);
        text(std::to_string(game.rightScore),565,78,48,orange);
        text("ROUNDS  "+std::to_string(game.leftRounds)+" / 2",40,78,14,cyan);
        text("ROUNDS  "+std::to_string(game.rightRounds)+" / 2",810,78,14,orange);
        for(std::size_t i=0;i<trail.size();++i) {
            sf::CircleShape dot(7*(1-float(i)/trail.size()));
            dot.setOrigin({dot.getRadius(),dot.getRadius()});dot.setPosition({trail[i].x,trail[i].y+65});
            dot.setFillColor(sf::Color(95,233,221,static_cast<unsigned char>(85*(1-float(i)/trail.size()))));window.draw(dot);
        }
        rect(44,game.leftY-48+65,12,96,cyan);rect(944,game.rightY-48+65,12,96,orange);
        sf::CircleShape ball(9);ball.setOrigin({9,9});ball.setPosition({game.x,game.y+65});window.draw(ball);
        text("W / S   MOVE",32,718,14,muted);
        text("P   PAUSE     R   RESTART     ESC   MENU",310,718,14,muted);
        text("BEST RALLY  "+std::to_string(game.bestRally),800,718,14,cyan);
        auto panel=[&]() {rect(150,220,700,300,sf::Color(9,15,25,245));};
        if(game.phase==updated::Phase::Menu) {
            panel();text("A CLASSIC. REBUILT.",220,245,38,sf::Color::White);
            text("1  SOLO VS AI       2  LOCAL TWO PLAYER",220,315,20,muted);
            text(std::string("MODE: ")+(solo?"SOLO":"TWO PLAYER")+"    /    D: "+(difficulty==0?"EASY":difficulty==1?"NORMAL":"HARD"),220,360,20,cyan);
            text("ENTER  /  START MATCH",220,420,25,orange);
            text("First to 5 points. Best of 3 rounds. Right player: arrow keys.",220,470,16,muted);
        } else if(game.phase==updated::Phase::Paused) {
            panel();text("MATCH PAUSED",250,275,38,sf::Color::White);
            text("P  RESUME    /    R  RESTART    /    ESC  MENU",220,385,20,cyan);
        } else if(game.phase==updated::Phase::MatchOver) {
            panel();text(game.leftRounds==2?"LEFT PLAYER WINS":"RIGHT PLAYER WINS",225,270,36,sf::Color::White);
            text("BEST RALLY  "+std::to_string(game.bestRally)+"    /    PADDLE HITS  "+std::to_string(game.hits),225,340,21,cyan);
            text("ENTER  REMATCH    /    ESC  MENU",225,420,22,orange);
        } else if(game.phase==updated::Phase::Serve) {
            text("READY",440,310,30,cyan);
        }
        window.display();
    }
}
