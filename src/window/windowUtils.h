#ifndef _windowutils_h
#define _windowutils_h

struct copyPaste {

    //---Analysis
    bool baseColor = false;
    bool cropPoints = false;
    bool analysisBlur = false;
    bool analysis = false;

    //---Grade
    bool temp = false;
    bool tint = false;
    bool bp = false;
    bool wp = false;
    bool lift = false;
    bool gain = false;
    bool mult = false;
    bool offset = false;
    bool gamma = false;

    //---Metadata
    bool make = false;
    bool model = false;
    bool lens = false;
    bool stock = false;
    bool focal = false;
    bool fstop = false;
    bool exposure = false;
    bool date = false;
    bool location = false;
    bool gps = false;
    bool notes = false;
    bool dev = false;
    bool chem = false;
    bool devnote = false;

    void analysisGlobal(){
        if (baseColor && cropPoints &&
            analysisBlur && analysis)
            baseColor = cropPoints = analysisBlur = analysis = !analysis;
        else
            baseColor = cropPoints = analysisBlur = analysis = true;
    }

    void gradeGlobal(){
        if (temp && tint && bp && wp && lift &&
            gain && mult && offset && gamma)
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = !gamma;
        else
            temp = tint = bp = wp = lift = gain = mult = offset = gamma = true;
    }

    void metaGlobal() {
        if (make && model && lens && stock && focal &&
            fstop && exposure && date && location &&
            gps && notes && dev && chem && devnote)
                make = model = lens = stock = focal =
                fstop = exposure = date = location =
                gps = notes = dev = chem = devnote = !devnote;
        else
            make = model = lens = stock = focal =
            fstop = exposure = date = location =
            gps = notes = dev = chem = devnote = true;
    }
};

bool ImRightAlign(const char* str_id);
#define ImEndRightAlign ImGui::EndTable


void transformCoordinates(int& x, int& y, int rotation, int width, int height);
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height);
#endif
