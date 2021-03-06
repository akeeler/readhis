/*
 * Copyright Krzysztof Miernik 2012
 * k.a.miernik@gmail.com 
 *
 * Distributed under GNU General Public Licence v3
 */

#include <string> 
#include <cmath> 
#include <iomanip>
#include "DrrBlock.h"
#include "HisDrr.h"
#include "Histogram.h"
#include "Exceptions.h"
#include "Options.h"
#include "HisDrrHisto.h"
#include "Polygon.h"
#include "Debug.h"

using namespace std;

HisDrrHisto::HisDrrHisto(const string drr, const string his, 
                         const Options* options)
                        : HisDrr(drr, his) {
    options_ = options;
}

void HisDrrHisto::runListMode(bool more) {
    vector<int> list;
    getHisList(list);
    cout.setf(ios::left, ios::adjustfield);
    cout << setw (12) << "# Histogram"
         << setw (7)  << "Empty?"
         << setw (4)  << "Dim"
         << setw (50) << "Title"
         << endl;
    for (vector<int>::iterator itl = list.begin();
            itl != list.end();
            ++itl) {
        char emptiness = '?';
        info = getHistogramInfo(*itl);
        if (more) {
            vector<unsigned int> d;
            getHistogram(d, *itl);
            bool empty = true;
            for (vector<unsigned int>::iterator it = d.begin();
                    it != d.end();
                    ++it) {
                if (*it > 0) {
                    empty = false;
                    break;
                }
            }
            if (empty)
                emptiness = 'Y';
            else
                emptiness = 'N';
        }
        // Removes junk characters at the end of the info.title 
        string title(info.title, sizeof(info.title) / sizeof(char));
        cout << setw (12) << info.hisID
             << setw (7)  << emptiness
             << setw (4)  << info.hisDim
             << setw (50) << title
             << endl;
    }
    cout << endl;
    cout.setf(ios::internal, ios::adjustfield);
}

void HisDrrHisto::runInfoMode() {
    cout << "#ID: " << info.hisID << endl;
    cout << "#hisDim: " << info.hisDim << endl;
    cout << "#halfWords: " << info.halfWords << endl;
    for (int j = 0; j < 4; ++j)
        cout << "#params[" << j << "]: " << info.params[j] << endl;
    for (int j = 0; j < 4; ++j)
        cout << "#raw[" << j << "]: " << info.raw[j] << endl;
    for (int j = 0; j < 4; ++j)
        cout << "#scaled[" << j << "]: " << info.scaled[j] << endl;
    for (int j = 0; j < 4; ++j)
        cout << "#minc[" << j << "]: " << info.minc[j] << endl;
    for (int j = 0; j < 4; ++j)
        cout << "#maxc[" << j << "]: " << info.maxc[j] << endl;

    cout << "#offset: " << info.offset << endl;
    cout << "#xlabel: " << info.xlabel << endl;
    cout << "#ylabel: " << info.ylabel << endl;

    for (int j = 0; j < 4; ++j)
        cout << "#calcon[" << j << "]: " << info.calcon[j] << endl;

    cout << "#title: " << info.title << endl;
}

void HisDrrHisto::process1D() {
    // maxc + 1 because drr has bins numbered 0 to maxc 
    // but size then is maxc+ 1
    histogram = new Histogram1D(info.minc[0], info.maxc[0] + 1,
                                info.scaled[0], "");

    vector<unsigned> data;
    data.reserve(info.scaled[0]);
    //Load data from his file
    getHistogram(data, info.hisID);
    histogram->setDataRaw(data);

    Histogram1D* h1 = dynamic_cast<Histogram1D*>(histogram);

    if (options_->getBin()) {
        Histogram1D* h1b;
        vector<unsigned> bin;
        options_->getBinning(bin);
        if (bin[0] > 1) {
            double binW = h1->getBinWidthX() * bin[0];
            h1b = h1->rebin(h1->getxMin(), h1->getxMax(), binW);
            (*h1) = (*h1b);
            delete h1b;
        } else if (bin[0] <= 0)
            throw GenError("HisDrrHisto::process1D : Wrong binning size.");

    }

    unsigned nth = 1;
    if (options_->getEvery()) {
        vector<unsigned> every;
        options_->getEveryN(every);
        nth = every[0];
    }
    
    cout << "#X  N  dN" << endl;
    unsigned sz = h1->getnBinX();
    if (options_->getZeroSup()) {
        for (unsigned i = 0; i < sz; i += nth)
            if ((*h1)[i] != 0 )
                cout << h1->getX(i) << " " << (*h1)[i] << " " 
                     << sqrt( (*h1)[i] ) << endl;
    } else {
        for (unsigned i = 0; i < sz; i += nth)
            cout << h1->getX(i) << " " << (*h1)[i] 
                 << " " << sqrt( (*h1)[i] )    << endl;
    }
    delete histogram;
}

void HisDrrHisto::process2Dgate() {
    Histogram2D* h2 = dynamic_cast<Histogram2D*>(histogram);

    // Resulting projection
    Histogram1D* proj;
    // Uncertainities are going to be stored in a separate histogram
    Histogram1D* projErr;

    bool gx = options_->getGx();
    bool gy = options_->getGy();
    bool bg = options_->getBg();
    bool sbg = options_->getSBg();

    vector<unsigned> gate;
    if (gx)
        options_->getGateX(gate);
    else if (gy)
        options_->getGateY(gate);

    if (gate.size() < 2)
        throw GenError("process2D: Not enough gate points");

    if (gx) {
        proj = h2->gateX(gate[0], gate[1]);
        projErr = h2->gateX(gate[0], gate[1]);
    } else {
        proj = h2->gateY(gate[0], gate[1]);
        projErr = h2->gateY(gate[0], gate[1]);
    }

    if (bg || sbg){
        //--gy/gx --bg
        vector<unsigned> bgr;
        options_->getBgGate(bgr);
        Histogram1D* projBg;

        if (bgr.size() >= 2) {
            if (gx)
                projBg = h2->gateX(bgr[0], bgr[1]);
            else
                projBg = h2->gateY(bgr[0], bgr[1]);

            *proj -= *projBg;
            *projErr += *projBg;
        } else
            throw GenError("process2D: Not enough background gate points");

        if (sbg) {
            //--gy/gx --sbg
            if (bgr.size() >= 4) {
                if (gx)
                    projBg = h2->gateX(bgr[2], bgr[3]);
                else
                    projBg = h2->gateY(bgr[2], bgr[3]);

                *proj -= *projBg;
                *projErr += *projBg;
            } else
                throw GenError("process2D: Not enough split background gate points");
        }

        delete projBg;
    }

    if (options_->getBin()) {
        // proj binned
        Histogram1D* projBin;
        Histogram1D* projErrBin;

        vector<unsigned> bin;
        options_->getBinning(bin);
        if (gx && bin[1] > 1) {
            double binW = proj->getBinWidthX() * bin[1];

            projBin = proj->rebin(proj->getxMin(),
                                  proj->getxMax(),
                                  binW);
            projErrBin = projErr->rebin(projErr->getxMin(),
                                        projErr->getxMax(), 
                                        binW);
            (*proj) = (*projBin);
            (*projErr) = (*projErrBin);
            
            delete projBin;
            delete projErrBin;
        } else if (gx && bin[1] <= 0)
            throw GenError("HisDrrHisto::process1D : Wrong binning size.");

        if (gy && bin[0] > 1) {
            double binW = proj->getBinWidthX() * bin[0];
            projBin = proj->rebin(proj->getxMin(),
                                  proj->getxMax(), 
                                  binW);
            projErrBin = projErr->rebin(projErr->getxMin(),
                                        projErr->getxMax(), 
                                        binW);
            (*proj) = (*projBin);
            (*projErr) = (*projErrBin);
            
            delete projBin;
            delete projErrBin;
        } else if (gy && bin[0] <= 0)
            throw GenError("HisDrrHisto::process1D : Wrong binning size.");

    }

    unsigned sz = proj->getnBinX();
    //We assume here that 0 counts came from l = 1 Poisson distribution
    for (unsigned i = 0; i < sz; ++i)
        if ((*projErr)[i] == 0)
            (*projErr)[i] = 1;

    unsigned nth = 1;
    if (options_->getEvery()) {
        vector<unsigned> every;
        options_->getEveryN(every);
        if (gx)
            nth = every[0];
        else
            nth = every[1];
    }

    cout << "#X  N  dN" << endl;
    for (unsigned i = 0; i < sz; i += nth)
        cout << proj->getX(i) << " " << (*proj)[i] << " " << sqrt((*projErr)[i]) << endl;

    delete projErr;
    delete proj;

}

void HisDrrHisto::process2Dpolygate() {
    // Polygon gate
    Histogram2D* h2 = dynamic_cast<Histogram2D*>(histogram);

    bool gx = options_->getGx();

    Polygon* polgate;

    string polFile = options_->getPolygon();
    int coma = polFile.find_last_of(",");
    if (coma != (int)string::npos ) {
        string file = polFile.substr(0, coma);
        string id = polFile.substr(coma + 1);
        cout << "# BAN file " << file << " ban id " << id << endl;
        polgate = new Polygon(file, atoi(id.c_str()));
    } else {
        polgate = new Polygon(polFile);
    }

    unsigned szX = h2->getnBinX();
    unsigned szY = h2->getnBinY();
    
    double min = 0;
    double max = 0;
    unsigned pSz = 0;
    if (gx) {
        min = h2->getyMin();
        max = h2->getyMax();
        pSz = szY;
    } else {
        min = h2->getxMin();
        max = h2->getxMax();
        pSz = szX;
    }
    Histogram1D* proj = new Histogram1D(min, max, pSz, "");

    double xlow, ylow, xhigh, yhigh;
    polgate->rectangle(xlow, ylow, xhigh, yhigh);

    // getiX and getiY safely return 0 or xmax if out of histogram range
    unsigned xmin = h2->getiX(xlow);
    unsigned xmax = h2->getiX(xhigh);
    unsigned ymin = h2->getiY(ylow);
    unsigned ymax = h2->getiY(yhigh);

    for (unsigned x = xmin; x < xmax; ++x)
    for (unsigned y = ymin; y < ymax; ++y) {
        if (polgate->pointIn(h2->getX(x), h2->getY(y)) ) {
            if (gx)
                proj->add(y, (*h2)(x,y));
            else
                proj->add(x, (*h2)(x,y));
        }
    }

    unsigned nth = 1;
    if (options_->getEvery()) {
        vector<unsigned> every;
        options_->getEveryN(every);
        if (gx)
            nth = every[0];
        else
            nth = every[1];
    }
    
    cout << "#X  N  dN" << endl;
    for (unsigned i = 0; i < pSz; i += nth) {
        cout << proj->getX(i) << " " << (*proj)[i];
        if ((*proj)[i] == 0)
            cout << " " << 1 << endl;
        else
            cout << " " << sqrt((*proj)[i]) << endl;
    }
                
    delete proj;
    delete polgate;
}

void HisDrrHisto::process2Dcrop() {
    // Double gate case
    Histogram2D* h2 = dynamic_cast<Histogram2D*>(histogram);

    vector<unsigned> gateX;
    vector<unsigned> gateY;
    options_->getGateX(gateX);
    options_->getGateY(gateY);
    if (gateX.size() < 2 || gateY.size() < 2)
        throw GenError("process2D: Not enough gate points");

    unsigned nbinX = gateX[1] - gateX[0];
    unsigned nbinY = gateY[1] - gateY[0];
    
    Histogram2D* h2crop = new Histogram2D(gateX[0], gateX[1],
                                            gateY[0], gateY[1],
                                            nbinX, nbinY,
                                            "");

    unsigned newX = 0;
    for (unsigned x = gateX[0]; x < gateX[1]; ++x) {
        unsigned newY = 0;
        for (unsigned y = gateY[0]; y < gateY[1]; ++y) {
            (*h2crop)(newX, newY) = (*h2)(x, y);
            ++newY;
        }
        ++newX;
    }

    (*h2) =(*h2crop);
    delete h2crop;

    if (options_->getBin()) {
        Histogram2D* h2b;
        vector<unsigned> bin;
        options_->getBinning(bin);

        if ( !(bin[0] <= 1 && bin[1] <= 1) && 
                (bin[0] > 0  && bin[1] > 0 )      ) {
            double binWX = h2->getBinWidthX() * bin[0];
            double binWY = h2->getBinWidthY() * bin[1];
            h2b = h2->rebin(h2->getxMin(), h2->getxMax(), 
                               h2->getyMin(), h2->getyMax(), 
                               binWX, binWY);
            (*h2) = (*h2b);
            delete h2b;
        } else
            throw GenError("HisDrrHisto::process2D : Wrong binning size.");
    }

    unsigned szX = h2->getnBinX();
    unsigned szY = h2->getnBinY();
    
    unsigned nXth = 1;
    unsigned nYth = 1;
    if (options_->getEvery()) {
        vector<unsigned> every;
        options_->getEveryN(every);
        nXth = every[0];
        nYth = every[1];
    }
    

    cout << "#X  Y  N" << endl;
    //Zero suppresion for 2d histo breaks file for gnuplot pm3d map
    if (options_->getZeroSup()) {
        for (unsigned x = 0; x < szX; x += nXth) 
            for (unsigned y = 0; y < szY; y += nYth)
                if ((*h2)(x,y) != 0 )
                    cout << h2->getX(x) << " " << h2->getY(y)  
                            << " " << (*h2)(x,y) << endl;
    } else {
        for (unsigned x = 0; x < szX; x += nXth) {
            for (unsigned y = 0; y < szY; y += nYth)
                cout << h2->getX(x) << " " << h2->getY(y)  
                    << " " << (*h2)(x,y) << endl;
            cout << endl;
        }
    }
}

void HisDrrHisto::process2Dnogates() {
    // No gates case
    Histogram2D* h2 = dynamic_cast<Histogram2D*>(histogram);
    
    // Rebinning (if applicable)
    if (options_->getBin()) {
        Histogram2D* h2b;
        vector<unsigned> bin;
        options_->getBinning(bin);

        if ( !(bin[0] <= 1 && bin[1] <= 1) && 
                (bin[0] > 0  && bin[1] > 0 )      ) {
            double binWX = h2->getBinWidthX() * bin[0];
            double binWY = h2->getBinWidthY() * bin[1];
            h2b = h2->rebin(h2->getxMin(), h2->getxMax(), 
                               h2->getyMin(), h2->getyMax(), 
                               binWX, binWY);
            (*h2) = (*h2b);
            delete h2b;
        } else
            throw GenError("HisDrrHisto::process2D : Wrong binning size.");
    }

    unsigned szX = h2->getnBinX();
    unsigned szY = h2->getnBinY();

    unsigned nXth = 1;
    unsigned nYth = 1;
    if (options_->getEvery()) {
        vector<unsigned> every;
        options_->getEveryN(every);
        nXth = every[0];
        nYth = every[1];
    }
    
    cout << "#X  Y  N" << endl;
    //Zero suppresion for 2d histo breaks file for gnuplot pm3d map
    //But might be useful anyway 
    if (options_->getZeroSup()) {
        for (unsigned x = 0; x < szX; x += nXth) 
            for (unsigned y = 0; y < szY; y += nYth)
                if ((*h2)(x,y) != 0 )
                    cout << h2->getX(x) << " " << h2->getY(y)  
                            << " " << (*h2)(x,y) << endl;
    } else {
        for (unsigned x = 0; x < szX; x += nXth) {
            for (unsigned y = 0; y < szY; y += nYth)
                cout << h2->getX(x) << " " << h2->getY(y)  
                    << " " << (*h2)(x,y) << endl;
            cout << endl;
        }
    }
}

void HisDrrHisto::process2D() {
    histogram = new Histogram2D(info.minc[0], info.maxc[0] + 1,
                                info.minc[1], info.maxc[1] + 1, 
                                info.scaled[0], info.scaled[1],
                                "");

    vector<unsigned> data;
    data.reserve( info.scaled[0] * info.scaled[1]);

    //Load data from his file
    getHistogram(data, info.hisID);
    histogram->setDataRaw(data);

    bool gx = options_->getGx();
    bool gy = options_->getGy();
    bool pg = options_->getPg();

    if ( (gx || gy) && !pg && !(gx && gy) ){
        process2Dgate();
    } else if ( (gx || gy) && pg && !(gx && gy)) {
        process2Dpolygate();
    } else if (gx && gy) {
        process2Dcrop();
    } else {
        process2Dnogates();
    }

    delete histogram;
}

void HisDrrHisto::process() {

    try {
        if ( options_->getListMode() )
            runListMode(false);
        else if (options_->getListModeZ())
            runListMode(true);
        else {
            if (!options_->isIdSet()) {
                throw GenError("Histogram id is required");
            }

            int hisId = options_->getHisId();
            info = getHistogramInfo(hisId);

            if (options_->getInfoMode()) { 
                runInfoMode();
            } else if (info.hisDim == 1) {
                process1D();
            } else if (info.hisDim == 2) {
                process2D();
            } else {
                throw GenError("Only 1 and 2 dimensional histograms are supported.");
            }
        }
    } catch (GenError &err) {
        cout << "Error: " << err.show() << endl;
        cout << "Run readhis --help for more information" << endl;
    }
}

HisDrrHisto::~HisDrrHisto() {
}
