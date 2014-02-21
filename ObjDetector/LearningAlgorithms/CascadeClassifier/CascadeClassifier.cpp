#include "LearningAlgorithms/CascadeClassifier/CascadeClassifier.h"
#include "LearningAlgorithms/CascadeClassifier/StageClassifiers/GentleAdaboost.h"
#include "LOG.h"
#include <cassert>

using std::ostream;
using std::endl;

/* parameter X: positive samples(files) in the front, negative samples afterwards */
void CascadeClassifier::Train(vector<vector<vector<double>>>& X, vector<bool>& y)
{
    assert(X.size() == y.size());

    int n_total = (int)y.size();
    int n_pos = (int)count(y.begin(), y.end(), true);
    int n_neg = (int)(n_total - n_pos);

    assert(n_pos <= n_neg);

    vector<vector<vector<double>>> samples_X(X.begin(), X.begin() + 2 * n_pos);
    vector<bool> samples_y(y.begin(), y.begin() + samples_X.size());

    FPR = 1.0;
    TPR = 1.0;

    int j = n_pos * 2;

    LOG_INFO("\tCascade stages begin");
    for (int i = 0; i < max_stages_num && FPR > FPR_target; i++)
    {
        LOG_INFO("\tCascade stage " << i);

        shared_ptr<StageClassifier> stage_classifier(new GentleAdaboost(TPR_min_perstage));

        LOG_INFO("\tTraining stage classifier");
        stage_classifier->Train(samples_X, samples_y);
        
        /* search ROC curve */
        stage_classifier->SearchTheta(X, y);
        LOG_INFO("\tStage classifier FPR = " << stage_classifier->FPR << ", TPR = " << stage_classifier->TPR);

        FPR *= stage_classifier->FPR;
        TPR *= stage_classifier->TPR;

        stage_classifiers.push_back(stage_classifier);

        /* renew negative samples */
        samples_X.erase(samples_X.begin() + n_pos, samples_X.end());
        samples_y.erase(samples_y.begin() + n_pos, samples_y.end());

        for (; j < n_total; j++)
        {
            if (Predict(X[j]) == true) // if false positive
            {
                samples_X.push_back(X[j]);
                samples_y.push_back(y[j]);
            }

            if (samples_y.size() == n_pos * 2)
                break;
        }
        if (samples_y.size() != n_pos * 2)
        {
            LOG_WARNING("\tRunning out of negative samples.");
            break;
        }
    }
    LOG_INFO("\tCascade stages end");
}

bool CascadeClassifier::Predict(vector<vector<double>>& x)
{
    for (int i = 0; i < stage_classifiers.size(); i++)
    {
        if (stage_classifiers[i]->Predict(x) < stage_classifiers[i]->theta)
            return false;
    }

    return true;
}

bool CascadeClassifier::Predict2(vector<vector<vector<double>>>& x)
{
    for (int i = 0; i < stage_classifiers.size(); i++)
    {
        if (stage_classifiers[i]->Predict2(x[i]) < stage_classifiers[i]->theta)
            return false;
    }

    return true;
}

void CascadeClassifier::GetFittedPatchIndexes(vector<vector<int>>& patch_indexes)
{
    for (int i = 0; i < stage_classifiers.size(); i++)
    {
        vector<int> patch_indexes_perstage;
        stage_classifiers[i]->GetFittedPatchIndexes(patch_indexes_perstage);
        patch_indexes.push_back(patch_indexes_perstage);
    }
}

void CascadeClassifier::Print()
{
    cout << "FPR: " << FPR << ", TPR:" << TPR << endl;
}