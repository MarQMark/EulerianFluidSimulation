#ifndef EULERIANFLUIDSIMULATION_VIEWABLE_H
#define EULERIANFLUIDSIMULATION_VIEWABLE_H

class Viewable {
public:
    Viewable() = default;
    virtual ~Viewable() = default;

    virtual void render() = 0;

    virtual void setVisible(const bool visible) {_visible = visible;}
    virtual bool isVisible() {return _visible;}

private:
    bool _visible = true;
};

#endif //EULERIANFLUIDSIMULATION_VIEWABLE_H