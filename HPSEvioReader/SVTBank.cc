//
//  SVTbank.cc
//  HPSEvioReader
//
//  Created by Maurik Holtrop on 1/2/19.
//

#include "SVTBank.h"

ClassImp(SVTBank);

void SVTBank::PrintBank(int print_leaves, int depth, int level){
    // Print the contents of this bank to depth. The level parameter keeps track of the depth.
    // If print_leaves = 0, no leaves are printed, else a maximum of print_leaves leave content is printed.

    string s;
    for(int i=0;i<level;i++) s.append("    ");
    level++;
    char opts[48];
    sprintf(opts,"N%03dL%03d",print_leaves,level);
    cout << s << "Bank: " << GetName() << "\t tags= [";
    for(unsigned short t: tags){
        cout<<t << ",";
    }
    cout << "] num = " << (int)num << endl;

    if(fStoreRaw){
        if(print_leaves && leafs->GetEntriesFast() ){
            cout << s << "-----------------------------------------------------------------------\n";
            for(int i=0; i< leafs->GetEntriesFast() && i<print_leaves; ++i){
                leafs->At(i)->Print(opts);
            }
            cout << endl;
        }
    }else{
        cout << s << "-----------------------------------------------------------------------\n";
        cout << s << "Raw data not stored.\n";
    }

    cout << s << "Number of headers stored: " << svt_headers.size() << endl;
    cout << s << "Number of tais stored: " << svt_tails.size() << endl;

    cout << s << "-----------------------------------------------------------------------\n";
    for( int i =0 ; i< svt_data.size(); ++i){
        cout << s << "apv:" << svt_data[i].head.apv << " chan:" << std::setw(3) << svt_data[i].head.chan <<
        " feb_id:" << int(svt_data[i].head.feb_id) << " hyb_id:" << svt_data[i].head.hyb_id << " ";
        cout << " Samples: [";
        for(int j=0;j < MAX_NUM_SVT_SAMPLES; ++j){
            printf("%5d",svt_data[i].samples[j]);
            if(j<MAX_NUM_SVT_SAMPLES-1) cout << ",";
        }
        cout << "]\n";
     }
}
