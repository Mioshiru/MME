#ifndef RME_EDITOR_INTERFACES_H
#define RME_EDITOR_INTERFACES_H

class Position;

// Interface für Map Ein-/Ausgabe
class IEditorIO {
public:
    virtual ~IEditorIO() = default;
    virtual bool saveMap(FileName filename, bool show_dialog) = 0;
    virtual bool exportMiniMap(FileName filename, int floor, bool displaydialog) = 0;
};

// Interface für komplexe Brush-Operationen
class IEditorActions {
public:
    virtual ~IEditorActions() = default;
    virtual void borderizeSelection() = 0;
    virtual void randomizeSelection() = 0;
    virtual void borderizeMap(bool show_dialog) = 0;
    virtual void randomizeMap(bool show_dialog) = 0;
};

// Interface für Selektions-Handling
class IEditorSelection {
public:
    virtual ~IEditorSelection() = default;
    virtual bool hasSelection() const = 0;
    virtual void moveSelection(Position offset) = 0;
    virtual void destroySelection() = 0;
};

#endif