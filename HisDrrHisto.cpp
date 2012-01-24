#include <string> 
#include "HisDrrHisto.h"
#include "Options.h"

using namespace std;

inline HisDrrHisto::HisDrrHisto(const Options* options, string& baseName) {
    options_ = options;
    baseName_ = baseName;
}

void runListMode(HisDrr* h, bool zero /* = false */) {
    vector<int> list;
    h->getHisList(list);
    unsigned numOfHis = list.size();
    for (unsigned int i = 0; i < numOfHis; ++i) {
        if (zero) {
            vector<unsigned int> d;
            h->getHistogram(d, list[i]);
            unsigned int sz = d.size();
            bool empty = true;
            for (unsigned int j = 0; j < sz; ++j)
                if (d[j] > 0) {
                    empty = false;
                    break;
                }
            if (empty)
                cout << "E" << list[i];
            else
                cout << "\033[1;34m" << list[i] << "\033[0m";
        } else {
            cout << list[i];
        }
        
        cout << ", ";
        if ((i+1) % 10 == 0)
            cout << endl;

    }
    cout << "\033[0;30m\033[0m" << endl;
    exit(0);
}

void runInfoMode(DrrHisRecordExtended& info) {
    cout << "# INFORMATIONS: " << info.hisID << endl;
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
    exit(0);
}

void HisDrrHisto::process() {

    try {
        string drr = baseName_ + ".drr";
        string his = baseName_ + ".his";

        HisDrr *hisdata = new HisDrr(drr, his);
        
        if ( options_->getListMode() )
            runListMode(hisdata);
        else if (options_->getListModeZ())
            runListMode(hisdata, true);

        int hisId = options_->getHisId();
        DrrHisRecordExtended info = hisdata->getHistogramInfo(hisId);

        if (options_->getInfoMode()) {
            runInfoMode(info);
        }

        if (info.hisDim == 1) {
            string name = "1D";
            Histogram1D* h1 = new Histogram1D(info.minc[0], info.maxc[0], info.scaled[0], name);

            vector<unsigned> data;
            data.reserve(info.scaled[0] + 2);
            //Load data from his file
            hisdata->getHistogram(data, hisId);
            // Now add under- and overshoot bins
            data.insert(data.begin(), 0);
            data.push_back(0);
            
            h1->setDataRaw(data);
            for (int i = 0; i < info.scaled[0] + 2; ++i)
                cout << h1->getX(i) << " " << (*h1)[i] << endl;

            delete h1;
        } else if (info.hisID == 2) {
            //Process here...
        } else {
            throw GenError("Error: Only 1 and 2 dimensional histograms are supported.");
        }

        delete hisdata;
    } catch (GenError &err) {
        cout << "Error: " << err.show() << endl;
        cout << "Run readhis --help for more information" << endl;
    }
}
