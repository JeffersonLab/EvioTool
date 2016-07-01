//
//  Bank_Linkdef.h
//  AprimeAna
//
//  Created by Maurik Holtrop on 5/19/14.
//  Copyright (c) 2014 University of New Hampshire. All rights reserved.
//

#ifndef AprimeAna_Bank_Linkdef_h
#define AprimeAna_Bank_Linkdef_h

#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class Bank+;
#pragma link C++ function Bank::Add_Leaf<int>;
#pragma link C++ function Bank::Add_Leaf<float>;
#pragma link C++ function Bank::Add_Leaf<double>;
#pragma link C++ function Bank::Add_Leaf<string>;
#pragma link C++ function Bank::Push_data<int>;
#pragma link C++ function Bank::Push_data<float>;
#pragma link C++ function Bank::Push_data<double>;
#pragma link C++ function Bank::Push_data<string>;
#pragma link C++ function Bank::Push_data_vector<int>;
#pragma link C++ function Bank::Push_data_vector<float>;
#pragma link C++ function Bank::Push_data_vector<double>;
#pragma link C++ function Bank::Push_data_vector<string>;
#pragma link C++ function Bank::Get_data<int>(int,int);
#pragma link C++ function Bank::Get_data<float>(int,int);
#pragma link C++ function Bank::Get_data<double>(int,int);
#pragma link C++ function Bank::Get_data<string>(int,int);
#pragma link C++ function Bank::Get_data<int>(string,int);
#pragma link C++ function Bank::Get_data<float>(string,int);
#pragma link C++ function Bank::Get_data<double>(string,int);
#pragma link C++ function Bank::Get_data<string>(string,int);
#pragma link C++ function Bank::Get_data_vector<int>;
#pragma link C++ function Bank::Get_data_vector<float>;
#pragma link C++ function Bank::Get_data_vector<double>;
#pragma link C++ function Bank::Get_data_vector<string>;

#pragma link C++ class vector<Bank>;

#endif


#endif
