
#include "treesV16Template.h"

struct stat info1;
struct stat info2;


#ifdef __MAKECINT__
#pragma link C++ class std::vector < std::vector<int> >+;
#pragma link C++ class std::vector < std::vector<float> >+;
#endif

using namespace std;

//Configurable parameters
int maxEvents=-1;

bool debug=false;

//Activated to display specific events
bool displayMode=false;

TTree * inTree;
TTree * outTree;

//FIXME: these should already be in treesV16Template.h


TString milliqanOfflineDir="/store/user/bmanley/";

void prepareOutBranches();
void clearOutBranches();
void getHSSyncRange(std::vector<hsData> &hsDataLoaded, std::vector<double> &dt_low, std::vector<double> &dt_high, int ifile, TString year);
int getHSSyncEvent(std::vector<hsData> &hsDataLoaded, std::vector<double> dt_low, std::vector<double> dt_high, double event_time_fromTDC);
void make_HS_treeV2(string runFile);

/*
# ifndef __CINT__  // the following code will be invisible for the interpreter
int main()
{
        gROOT->ProcessLine(".L make_HS_treeV2.C++");
        make_HS_treeV2();
}
# endif
*/

void make_HS_treeV2(string runFile){
    // Grab the directory from an input text file

        ifstream inF;
        string inputFileName = "";
        if(true) goto SKIP;
        inF.open(runFile);
        if(!inF.is_open()){
            cout << "Error: file with run directory could not be opened" << endl;
            return;
        }

        inF >> inputFileName;
        cout << "input: " << inputFileName<< endl;
        inF.close();
        SKIP: ;
        TString fileName = toTstring(runFile);
        TString year = "2018";
        TString version = "16";
        int eventNum=-1;
        TString tag="";
        float rangeMinX=-1.;
        float rangeMaxX=-1.;
        float rangeMinY=-1000.0;
        float rangeMaxY=-1000.0;
        int displayPulseBounds=0;
        int onlyForceChans=0;
        int runFFT=0;
        int applyLPFilter = 1;
        int injectPulses=0;
        float injectSignalQ=-1;

        if (injectPulses) cout << "Running in pulse injection mode!" << endl;
        if (injectSignalQ > 0) cout << "Running in signal injection mode with Q = " << injectSignalQ << "!" << endl;
        if (applyLPFilter) cout << "Applying low pass filter to waveforms!" << endl;

        TString inFileName = fileName;
        cout << inFileName << endl;
        ifstream inF2;
        inF2.open(inFileName);
        if(!inF2.is_open()){
            cout << "Error: file" << inFileName << " does not exist" << endl;
            return;
        }
        inF2.close();
        TFile *f = TFile::Open(inFileName, "READ");

        inTree = (TTree*)f->Get("t");


        TString baseFileName= ((TObjString*)inFileName.Tokenize("/")->Last())->String().Data();

        baseFileName= baseFileName(3,baseFileName.Length());

//Get run number from file name

        TString runNumber = baseFileName(baseFileName.First("_") + 4,  4);

        runNumber.ReplaceAll("MilliQan_Run","");
        runNum=atoi(runNumber.Data());
        runNumOrig = runNum;

//Get file number from file name
        TString fileNumber = ((TObjString*)baseFileName.Tokenize(".")->At(1))->String().Data();
        fileNumber = ((TObjString*)fileNumber.Tokenize("_")->At(0))->String().Data();
        fileNum=atoi(fileNumber.Data());

//Get config name from filename
        TString configName = ((TObjString*)baseFileName.Tokenize(".")->At(1))->String().Data();
        configName = ((TObjString*)configName.Tokenize("_")->At(1))->String().Data();
        configName.ReplaceAll(".root","");

        baseFileName="UX5"+baseFileName;
        baseFileName.ReplaceAll(".root","");

//TString version = GetStdoutFromCommand("git describe --tag --abbrev=0"); //fixme- this is currently evaluated at runtime, instead of compile time
        if(version.Contains("placeholder")) {cout<<"This macro was compiled incorrectly. Please compile this macro using compile.sh"<<endl; return;}

        int offsetSignalQ = 0;

        //gSystem->Exec("mkdir /store/user/bmanley/HS_trees/");
        //gSystem->Exec("mkdir /store/user/bmanley/HS_trees_v16/");
        //gSystem->Exec("mkdir /store/user/bmanley/HS_trees/HS_Run"+to_string(runNum)+"_"+configName);
        //gSystem->Exec("mkdir /store/user/bmanley/HS_trees_v16/HS_Run"+to_string(runNum)+"_"+configName);


        TString localDir = "/home/bmanley/";
        TString treeDirectory= milliqanOfflineDir+"HS_trees_v"+version+"/HS_Run"+to_string(runNum)+"_"+configName+"/";
        TString linkDirectory= milliqanOfflineDir+"HS_trees/HS_Run"+to_string(runNum)+"_"+configName+"/";
        //TString treeDirectory = "HS_Run"+to_string(runNum)+"_"+configName+"/";
        //TString linkDirectory = "HS_Run"+to_string(runNum)+"_"+configName+"/";

        if (injectPulses) baseFileName.ReplaceAll(runNumber,"-"+runNumber);
        if (injectSignalQ > 0) baseFileName.ReplaceAll(runNumber,to_string(runNum));
        //TString outFileName = treeDirectory+baseFileName+".root";
        TString outFileName = baseFileName+".root";

        cout<<"Run "<<runNum<<", file "<<fileNum<<endl;

        if(maxEvents<0) maxEvents=inTree->GetEntries();
        if(!displayMode) cout<<"Entries: "<<inTree->GetEntries()<<endl;


        cout << outFileName << endl;
        TFile *  outFile = new TFile(outFileName, "RECREATE");
        outTree = new TTree("t", "t");
        prepareOutBranches();

// hodoscope //FIXME
        std::vector<hsData> hsDataLoaded;
// deltaT (hs time, mq time) ranges
        std::vector<double> dt_low;
        std::vector<double> dt_high;
        getHSSyncRange(hsDataLoaded,dt_low,dt_high,fileNum, year);

// hodoscope //FIXME

        //debug = true;

        if(debug) maxEvents=100;
        cout<<"Starting event loop"<<endl;
        for(int i=0; i<maxEvents; i++) {
                if(i%200==0) cout<<"Processing event "<<i<<endl;
                inTree->GetEntry(i);
                SetAddressesTree(inTree);

                int sync_hs_event_index = getHSSyncEvent(hsDataLoaded,dt_low,dt_high,event_time_fromTDC);

                if (sync_hs_event_index>0)
                {

                        for(unsigned int j=0; j<hsDataLoaded.at(sync_hs_event_index).getHSData().size(); j++)
                        {
                                if(hsDataLoaded.at(sync_hs_event_index).getHSData().at(j)==1) v_hs->push_back(j);
                        }
                        for(unsigned int j=0; j<hsDataLoaded.at(sync_hs_event_index).getTPData().size(); j++)
                        {
                                if(hsDataLoaded.at(sync_hs_event_index).getTPData().at(j)==1) v_tp->push_back(j);
                        }
                        hs_time = hsDataLoaded.at(sync_hs_event_index).getMicroTime();
                        extrg   = hsDataLoaded.at(sync_hs_event_index).isMQEvent();
                }
                else
                {
                        extrg   =  0;
                        hs_time = -1;
                        v_hs->push_back(-1);
                        v_tp->push_back(-1);
                }



               if (sync_hs_event_index>0)
               {
               // Tracking
               // get fit parameter

               //original
               //vector<double> parFit = doFit(hs_data);

                //test
                vector<double> parFit = doFit(v_hs);



               if(parFit.size()>0)
               {
                 //cout << parFit[0] << " " << parFit[1] << endl;
                 //cout << parFit[2] << " " << parFit[3] << endl;

                 v_fit_xz->push_back(parFit[0]);
                 v_fit_xz->push_back(parFit[1]);
                 v_fit_yz->push_back(parFit[2]);
                 v_fit_yz->push_back(parFit[3]);
               }
               parFit.clear();
               }

            // hodoscope data //FIXME //here

           // cout << event << " " << event_time_fromTDC << endl;


                //event=i;
                outTree->Fill();
                clearOutBranches();
                fillNum=0;
                beam=false;
                hardNoBeam=false;
                fillAvgLumi=-1;
                fillTotalLumi=-1;
        }
        if(!displayMode) cout<<"Processed "<<maxEvents<<" events."<<endl;

        if(!displayMode) {
                outTree->Write();
                outFile->Close();

                cout<<"Closed output tree."<<endl;
                //TString currentDir=gSystem->pwd();
                //TString target = currentDir+"/"+outFileName;
                TString target = outFileName;
                TString linkname =linkDirectory+baseFileName+".root";
                remove(linkname); //remove if already exists
                //gSystem->Symlink(target,linkname);
                //cout<<"Made link to "<<target<<" called "<<linkname<<endl;
        }

        f->Close();
}


void prepareOutBranches(){

  //MAKE SURE TO ALWAYS ADD BRANCHES TO CLEAR FUNCTION AS WELL

        TBranch * b_event = outTree->Branch("event",&event,"event/I");
        TBranch * b_run = outTree->Branch("run",&runNumOrig,"run/I");
        TBranch * b_file = outTree->Branch("file",&fileNum,"file/I");
        TBranch * b_fill = outTree->Branch("fill",&fillNum,"fill/I");
        TBranch * b_nRollOvers = outTree->Branch("nRollOvers",&nRollOvers,"nRollOvers/I");
        TBranch * b_beam = outTree->Branch("beam",&beam,"beam/O");
        TBranch * b_hardNoBeam = outTree->Branch("hardNoBeam",&hardNoBeam,"hardNoBeam/O");
        TBranch * b_fillAvgLumi = outTree->Branch("fillAvgLumi",&fillAvgLumi,"fillAvgLumi/F");
        TBranch * b_fillTotalLumi = outTree->Branch("fillTotalLumi",&fillTotalLumi,"fillTotalLumi/F");
        TBranch * b_present_b0 = outTree->Branch("present_b0",&present_b0,"present_b0/O");
        TBranch * b_present_b1 = outTree->Branch("present_b1",&present_b1,"present_b1/O");
        TBranch * b_event_time_b0 = outTree->Branch("event_time_b0",&event_time_b0,"event_time_b0/L");
        TBranch * b_event_time_b1 = outTree->Branch("event_time_b1",&event_time_b1,"event_time_b1/L");
        TBranch * b_event_time_fromTDC = outTree->Branch("event_time_fromTDC",&event_time_fromTDC,"event_time_fromTDC/D");
        TBranch * b_t_since_fill_start = outTree->Branch("t_since_fill_start",&t_since_fill_start,"t_since_fill_start/I");
        TBranch * b_t_since_fill_end = outTree->Branch("t_since_fill_end",&t_since_fill_end,"t_since_fill_end/I");
        TBranch * b_t_until_next_fill = outTree->Branch("t_until_next_fill",&t_until_next_fill,"t_until_next_fill/I");
        TBranch * b_event_t_string = outTree->Branch("event_t_string",&event_t_string);
        TBranch * b_event_trigger_time_tag_b0 = outTree->Branch("event_trigger_time_tag_b0",&event_trigger_time_tag_b0,"event_trigger_time_tag_b0/L");
        TBranch * b_event_trigger_time_tag_b1 = outTree->Branch("event_trigger_time_tag_b1",&event_trigger_time_tag_b1,"event_trigger_time_tag_b1/L");

        TBranch * b_chan = outTree->Branch("chan",&v_chan);
        TBranch * b_triggerCandidates = outTree->Branch("triggerCandidates",&v_triggerCandidates);
        TBranch * b_triggerCandidatesEnd = outTree->Branch("triggerCandidatesEnd",&v_triggerCandidatesEnd);
        TBranch * b_triggerCandidatesChannel = outTree->Branch("triggerCandidatesChannel",&v_triggerCandidatesChannel);
        TBranch * b_layer = outTree->Branch("layer",&v_layer);
        TBranch * b_row = outTree->Branch("row",&v_row);
        TBranch * b_column = outTree->Branch("column",&v_column);
        TBranch * b_type = outTree->Branch("type",&v_type);
        TBranch * b_height = outTree->Branch("height",&v_height);
        TBranch * b_time = outTree->Branch("time",&v_time);
        TBranch * b_time_module_calibrated = outTree->Branch("time_module_calibrated",&v_time_module_calibrated);
        TBranch * b_delay = outTree->Branch("delay",&v_delay);
        TBranch * b_area = outTree->Branch("area",&v_area);
        TBranch * b_nPE = outTree->Branch("nPE",&v_nPE);
        TBranch * b_ipulse = outTree->Branch("ipulse",&v_ipulse);
        TBranch * b_npulses = outTree->Branch("npulses",&v_npulses);
        TBranch * b_duration = outTree->Branch("duration",&v_duration);
        TBranch * b_quiet = outTree->Branch("quiet",&v_quiet);
        TBranch * b_presample_mean = outTree->Branch("presample_mean",&v_presample_mean);
        TBranch * b_presample_RMS = outTree->Branch("presample_RMS",&v_presample_RMS);
        TBranch * b_sideband_mean = outTree->Branch("sideband_mean",&v_sideband_mean);
        TBranch * b_sideband_RMS = outTree->Branch("sideband_RMS",&v_sideband_RMS);
        TBranch * b_triggerBand_mean = outTree->Branch("triggerBand_mean",&v_triggerBand_mean);
        TBranch * b_triggerBand_max = outTree->Branch("triggerBand_max",&v_triggerBand_max);
        TBranch * b_triggerBand_maxTime = outTree->Branch("triggerBand_maxTime",&v_triggerBand_maxTime);
        TBranch * b_triggerBand_RMS = outTree->Branch("triggerBand_RMS",&v_triggerBand_RMS);
        TBranch * b_sideband_mean_calib = outTree->Branch("sideband_mean_calib",&v_sideband_mean_calib);
        TBranch * b_sideband_RMS_calib = outTree->Branch("sideband_RMS_calib",&v_sideband_RMS_calib);

        TBranch * b_groupTDC_b0 = outTree->Branch("groupTDC_b0",&v_groupTDC_b0);
        TBranch * b_groupTDC_b1 = outTree->Branch("groupTDC_b1",&v_groupTDC_b1);

        TBranch * b_max = outTree->Branch("max",&v_max);
        TBranch * b_min = outTree->Branch("min",&v_min);
        TBranch * b_maxAfterFilter = outTree->Branch("maxAfterFilter",&v_maxAfterFilter);
        TBranch * b_maxThreeConsec = outTree->Branch("maxThreeConsec",&v_maxThreeConsec);
        TBranch * b_minAfterFilter = outTree->Branch("minAfterFilter",&v_minAfterFilter);

        TBranch * b_bx = outTree->Branch("bx",&v_bx);
        TBranch * b_by = outTree->Branch("by",&v_by);
        TBranch * b_bz = outTree->Branch("bz",&v_bz);

        // hodoscope // FIXME
        TBranch * b_extrg   = outTree->Branch("extrg",&extrg,"extrg/I");
        TBranch * b_hs_time = outTree->Branch("hs_time",&hs_time,"hs_time/D");
        TBranch * b_v_hs      = outTree->Branch("v_hs",&v_hs);
        TBranch * b_v_tp      = outTree->Branch("v_tp",&v_tp);
        TBranch * b_v_fit_xz  = outTree->Branch("v_fit_xz",&v_fit_xz);
        TBranch * b_v_fit_yz  = outTree->Branch("v_fit_yz",&v_fit_yz);


        outTree->SetBranchAddress("event",&event,&b_event);
        outTree->SetBranchAddress("run",&runNumOrig,&b_run);
        outTree->SetBranchAddress("file",&fileNum,&b_file);
        outTree->SetBranchAddress("nRollOvers",&nRollOvers,&b_nRollOvers);
        outTree->SetBranchAddress("event_time_b0",&event_time_b0,&b_event_time_b0);
        outTree->SetBranchAddress("event_time_b1",&event_time_b1,&b_event_time_b1);
        outTree->SetBranchAddress("event_time_fromTDC",&event_time_fromTDC,&b_event_time_fromTDC);
        outTree->SetBranchAddress("t_since_fill_start",&t_since_fill_start,&b_t_since_fill_start);
        outTree->SetBranchAddress("t_since_fill_end",&t_since_fill_end,&b_t_since_fill_end);
        outTree->SetBranchAddress("t_until_next_fill",&t_until_next_fill,&b_t_until_next_fill);
        outTree->SetBranchAddress("fill",&fillNum,&b_fill);
        outTree->SetBranchAddress("beam",&beam,&b_beam);
        outTree->SetBranchAddress("hardNoBeam",&hardNoBeam,&b_hardNoBeam);
        outTree->SetBranchAddress("fillAvgLumi",&fillAvgLumi,&b_fillAvgLumi);
        outTree->SetBranchAddress("fillTotalLumi",&fillTotalLumi,&b_fillTotalLumi);
        outTree->SetBranchAddress("present_b0",&present_b0,&b_present_b0);
        outTree->SetBranchAddress("present_b1",&present_b1,&b_present_b1);
        outTree->SetBranchAddress("event_trigger_time_tag_b0",&event_trigger_time_tag_b0,&b_event_trigger_time_tag_b0);
        outTree->SetBranchAddress("event_trigger_time_tag_b1",&event_trigger_time_tag_b1,&b_event_trigger_time_tag_b1);

        //outTree->SetBranchAddress("event_t_string",&event_t_string,&b_event_t_string);
        outTree->SetBranchAddress("chan",&v_chan,&b_chan);
        outTree->SetBranchAddress("triggerCandidates",&v_triggerCandidates,&b_triggerCandidates);
        outTree->SetBranchAddress("triggerCandidatesEnd",&v_triggerCandidatesEnd,&b_triggerCandidatesEnd);
        outTree->SetBranchAddress("triggerCandidatesChannel",&v_triggerCandidatesChannel,&b_triggerCandidatesChannel);
        outTree->SetBranchAddress("layer",&v_layer,&b_layer);
        outTree->SetBranchAddress("row",&v_row,&b_row);
        outTree->SetBranchAddress("column",&v_column,&b_column);
        outTree->SetBranchAddress("type",&v_type,&b_type);
        outTree->SetBranchAddress("height",&v_height,&b_height);
        outTree->SetBranchAddress("time_module_calibrated",&v_time_module_calibrated,&b_time_module_calibrated);
        outTree->SetBranchAddress("time",&v_time,&b_time);
        outTree->SetBranchAddress("delay",&v_delay,&b_delay);
        outTree->SetBranchAddress("area",&v_area,&b_area);
        outTree->SetBranchAddress("nPE",&v_nPE,&b_nPE);
        outTree->SetBranchAddress("ipulse",&v_ipulse,&b_ipulse);
        outTree->SetBranchAddress("npulses",&v_npulses,&b_npulses);
        outTree->SetBranchAddress("duration",&v_duration,&b_duration);
        outTree->SetBranchAddress("quiet",&v_quiet,&b_quiet);
        outTree->SetBranchAddress("presample_mean",&v_presample_mean,&b_presample_mean);
        outTree->SetBranchAddress("presample_RMS",&v_presample_RMS,&b_presample_RMS);
        outTree->SetBranchAddress("sideband_mean",&v_sideband_mean,&b_sideband_mean);
        outTree->SetBranchAddress("sideband_RMS",&v_sideband_RMS,&b_sideband_RMS);
        outTree->SetBranchAddress("triggerBand_mean",&v_triggerBand_mean,&b_triggerBand_mean);
        outTree->SetBranchAddress("triggerBand_max",&v_triggerBand_max,&b_triggerBand_max);
        outTree->SetBranchAddress("triggerBand_maxTime",&v_triggerBand_maxTime,&b_triggerBand_maxTime);
        outTree->SetBranchAddress("triggerBand_RMS",&v_triggerBand_RMS,&b_triggerBand_RMS);
        outTree->SetBranchAddress("sideband_mean_calib",&v_sideband_mean_calib,&b_sideband_mean_calib);
        outTree->SetBranchAddress("sideband_RMS_calib",&v_sideband_RMS_calib,&b_sideband_RMS_calib);

        outTree->SetBranchAddress("groupTDC_b0",&v_groupTDC_b0,&b_groupTDC_b0);
        outTree->SetBranchAddress("groupTDC_b1",&v_groupTDC_b1,&b_groupTDC_b1);
        outTree->SetBranchAddress("max",&v_max,&b_max);
        outTree->SetBranchAddress("maxAfterFilter",&v_maxAfterFilter,&b_maxAfterFilter);
        outTree->SetBranchAddress("maxThreeConsec",&v_maxThreeConsec,&b_maxThreeConsec);
        outTree->SetBranchAddress("minAfterFilter",&v_minAfterFilter,&b_minAfterFilter);

        outTree->SetBranchAddress("bx",&v_bx,&b_bx);
        outTree->SetBranchAddress("by",&v_by,&b_by);
        outTree->SetBranchAddress("bz",&v_bz,&b_bz);

        // hodoscope // FIXME
        outTree->SetBranchAddress("extrg",    &extrg,     &b_extrg);
        outTree->SetBranchAddress("hs_time",  &hs_time,   &b_hs_time);
        outTree->SetBranchAddress("v_hs",     &v_hs,      &b_v_hs);
        outTree->SetBranchAddress("v_tp",     &v_tp,      &b_v_tp);
        outTree->SetBranchAddress("v_fit_xz", &v_fit_xz,  &b_v_fit_xz);
        outTree->SetBranchAddress("v_fit_yz", &v_fit_yz,  &b_v_fit_yz);
}

void clearOutBranches(){
  v_max->clear();
        v_maxAfterFilter->clear();
        v_maxThreeConsec->clear();
        v_min->clear();
        v_minAfterFilter->clear();
        v_chan->clear();
        v_triggerCandidates->clear();
        v_triggerCandidatesEnd->clear();
        v_triggerCandidatesChannel->clear();
        v_layer->clear();
        v_row->clear();
        v_column->clear();
        v_type->clear();
        v_height->clear();
        v_time->clear();
        v_time_module_calibrated->clear();
        v_area->clear();
        v_nPE->clear();
        v_ipulse->clear();
        v_npulses->clear();
        v_duration->clear();
        v_delay->clear();
        v_sideband_mean->clear();
        v_sideband_RMS->clear();
        v_triggerBand_mean->clear();
        v_triggerBand_max->clear();
        v_triggerBand_maxTime->clear();
        v_triggerBand_RMS->clear();
        v_sideband_mean_calib->clear();
        v_sideband_RMS_calib->clear();
        v_quiet->clear();
        v_presample_mean->clear();
        v_presample_RMS->clear();
        v_groupTDC_b0->clear();
        v_groupTDC_b1->clear();
        v_bx->clear();
        v_by->clear();
        v_bz->clear();
        // hodoscope //FIXME
        v_hs->clear();
        v_tp->clear();
        v_fit_xz->clear();
        v_fit_yz->clear();


}

void getHSSyncRange(std::vector<hsData> &hsDataLoaded, std::vector<double> &dt_low, std::vector<double> &dt_high, int ifile, TString year) {
        // get mq time stamp in the mq first event
        double event_time_fromTDC_evt0 = -1;

        SetAddressesTree(inTree);
        for(int i=0; i<10; i++) {
                inTree->GetEntry(i);
                if(event==0) event_time_fromTDC_evt0 = event_time_fromTDC;
        }


        // ------------------------------------------------------------------------------------
        //  Find HS files to look at
        // ------------------------------------------------------------------------------------
        // 1 hour before

        time_t epoch_previous_file    = static_cast<int>(event_time_fromTDC_evt0-3600);

        struct tm *date_previous_file = gmtime(&epoch_previous_file);
        TString hs_previous_file = Form("data_%d%02d%02d%02d.txt", date_previous_file->tm_year+1900, date_previous_file->tm_mon+1, date_previous_file->tm_mday,   date_previous_file->tm_hour);
        // 1 hour after
        time_t epoch_next_file    = static_cast<int>(event_time_fromTDC_evt0+3600);
        struct tm *date_next_file = gmtime(&epoch_next_file);
        TString hs_next_file = Form("data_%d%02d%02d%02d.txt", date_next_file->tm_year+1900, date_next_file->tm_mon+1, date_next_file->tm_mday, date_next_file->tm_hour);
        // current
        time_t epoch    = static_cast<int>(event_time_fromTDC_evt0);
        struct tm *date = gmtime(&epoch);
        TString hs_current_file = Form("data_%d%02d%02d%02d.txt", date->tm_year+1900, date->tm_mon+1, date->tm_mday, date->tm_hour);

        std::cout << "HS files in consideration: " << std::endl;
        std::cout << hs_previous_file << std::endl;
        std::cout << hs_current_file << std::endl;
        std::cout << hs_next_file << std::endl;

        std::cout <<  Form("Time: %d%02d%02d %02d:%02d:%02d", date->tm_year+1900, date->tm_mon+1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec) <<    std::endl;

        TString hs_file_dir="/store/user/llavezzo/HSdata/" + year;

        // ------------------------------------------------------------------------------------
        //  Store HS data in memory
        // ------------------------------------------------------------------------------------
        // 1. if near boundary then look at two adjacent files
        if(date->tm_min<3 && ifile>1)
        {
                loadHSData(hs_file_dir, hs_previous_file, hsDataLoaded, -1);
        }
        // 2. load events in the main file
        loadHSData(hs_file_dir, hs_current_file, hsDataLoaded, 0);

        // 3. if the end time is in the next file, add events to memory
        if(date->tm_min>57)
        {
                loadHSData(hs_file_dir, hs_next_file, hsDataLoaded, 1);
        }

        std::cout << "Number of loaded HS events: " << hsDataLoaded.size() << std::endl;

        // make a histogram of delta(hs time, mq time)
        TH1D *h1_dt  = new TH1D("h1_dt", "h1_dt", 100000, -10, 10);
        double mq_current_time=event_time_fromTDC_evt0;
        double mq_previous_time=event_time_fromTDC_evt0;
        for(int i=0; i<maxEvents; i++) {
                inTree->GetEntry(i);
                SetAddressesTree(inTree);

                mq_current_time = event_time_fromTDC;

                // Loop over selected HS events
                double hs_current_time=0;
                double hs_previous_time=0;
                for(unsigned int ihs=0; ihs<hsDataLoaded.size(); ihs++)
                {
                        // only extrg events
                        //if(!hsDataLoaded.at(ihs).isMQEvent()) continue;

                        hs_current_time = hsDataLoaded.at(ihs).getMicroTime();
                        cout << std::fixed;

                        double dt_of_dt = (mq_current_time-mq_previous_time) - (hs_current_time-hs_previous_time);
                        dt_of_dt = 0; // not using dt_of_dt in the syncing decision
                        if(TMath::Abs(dt_of_dt)<0.00001) h1_dt->Fill(mq_current_time-hs_current_time);

                        hs_previous_time = hs_current_time;
                }

                mq_previous_time = mq_current_time;

        }

        // extract dT ranges
        for(int i=1; i<=h1_dt->GetXaxis()->GetNbins(); i++)
        {
                if(h1_dt->GetBinContent(i)>100)
                {
                        double range_low = h1_dt->GetBinLowEdge(i-2);
                        double range_high = h1_dt->GetBinLowEdge(i+2);
                        std::cout << "dT synch range: " << range_low << " - " << range_high << std::endl;
                        dt_low.push_back(range_low);
                        dt_high.push_back(range_high);
                }
        }

}

int getHSSyncEvent(std::vector<hsData> &hsDataLoaded, std::vector<double> dt_low, std::vector<double> dt_high, double event_time_fromTDC) {

        double mq_current_time = event_time_fromTDC;

        // Get the window of HS events to compare
        //double hs_begin_time = hsDataLoaded.at(0).getMicroTime();
        //double hs_end_time = hsDataLoaded.at(hsDataLoaded.size()-1).getMicroTime();
        //double hs_total_time = hs_end_time-hs_begin_time;

        //int sync_window_sec = 30;

        // get the indices for synching window
        //int first_hs_evt = hsDataLoaded.size() * (mq_current_time-sync_window_sec-hs_begin_time)/hs_total_time;
        //int last_hs_evt  = hsDataLoaded.size() * (mq_current_time+sync_window_sec-hs_begin_time)/hs_total_time;

        int synched=-1;
        // Loop over selected HS events
        double hs_current_time=0;
        //for(int ihs=first_hs_evt; ihs<=last_hs_evt; ++ihs)
        for(unsigned int ihs=0; ihs<hsDataLoaded.size(); ++ihs)
        {
                hs_current_time = hsDataLoaded.at(ihs).getMicroTime();

                for(unsigned int irange=0; irange<dt_low.size(); irange++)
                {
                        //cout << dt_low.at(irange) << " " << dt_high.at(irange) << " " << mq_current_time-hs_current_time << endl;
                        if( mq_current_time-hs_current_time>dt_low.at(irange) &&
                            mq_current_time-hs_current_time<dt_high.at(irange)
                            ) synched = ihs;
                }
                if(synched>0)
                {
                        break;
                }
        }

        return synched;
}
