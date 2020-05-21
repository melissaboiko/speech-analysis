//
// Created by rika on 16/11/2019.
//

#include "../Analyser.h"
#include "LPC/Frame/LPC_Frame.h"
#include "Math/Polynomial.h"

using namespace Eigen;

void Analyser::analyseFormant() {
    static Formant::Frame defaultFrame;
    defaultFrame.nFormants = 5;
    defaultFrame.formant = {{550, 60}, {1650, 60}, {2750, 60}, {3850, 60}, {4950, 60}};
    defaultFrame.intensity = 1.0;

    if (lpFailed) {
        lastFormantFrame = defaultFrame;
        return;
    }

    switch (formantMethod) {
        case LP:
            LPC::toFormantFrame(lpcFrame, lastFormantFrame, fs);
            break;
        case KARMA:
            analyseFormantEkf();
            break;
        case DeepFormants:
            break;
    }
}

void Analyser::analyseFormantEkf() {

    const int numF = ekfState.numF;
   
    ekfState.voiced = (lastPitchFrame != 0);
    ekfState.fs = this->fs;

    EKF::step(ekfState);
   
    Formant::Frame frm;

    frm.nFormants = numF;
    frm.formant.resize(numF);

    for (int i = 0; i < numF; ++i) {
        frm.formant[i].frequency = ekfState.m_up(i);
        frm.formant[i].bandwidth = ekfState.m_up(numF + i);
    }

    Formant::sort(frm);

    lastFormantFrame = std::move(frm);

}
