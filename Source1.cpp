

#include "rapidjson/document.h" 
#include "rapidjson/pointer.h"
#include "rapidjson/filereadstream.h" 
#include "rapidjson/filewritestream.h" 
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" // for stringify JSON
#include <string>
#include <iostream> 
using namespace std;
using namespace rapidjson;

// RapidJSON can be found at https://rapidjson.org/
// Orginal files can be found data.gov.ie
// They were 'shrunk' at mapshaper.org - and the file names were changed when files were exported to preserve originals

// The concept was to merge two files together using a primary/foreign key for futher use in mapping
// The original file takes the Irish electoral districts and the second file takes the small area's with in those ED's
// and creates an out put - ["ED_ID" : { ED : {Properties of the ED}, SMA : [Array of Small areas (if available) - both propertie and geometry]},...]

int main()
{
    // Open the file for reading 
    FILE* fp = fopen("CSO_Electoral_Divisions2022Generalised50m.geojson", "r");
   
    // Use a FileReadStream to 
      // read the data from the file 
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer,
        sizeof(readBuffer));
  
    // Parse the JSON data  
      // using a Document object 
    rapidjson::Document document;
    document.ParseStream(is);
    std::cout << "finished open and parse " << std::endl;
    // Close the file 
    fclose(fp);

    rapidjson::Document newdoc;  // This the document for output

    Value* features = rapidjson::Pointer("/features").Get(document);
    newdoc.SetObject();
     

    for (auto& features : features->GetArray()) {
        // this gets the ed values for original cso file
        Value& props = features["properties"];
        // this is the primary key that links with the other file
        //Value* ed = rapidjson::Pointer("/ED_ID").Get(props); - Depends on the what department the file came from (and year) 
        Value* edCodeString = rapidjson::Pointer("/ED_ID_STR").Get(props);
        
        rapidjson::Value key;
 
        char buffer2[10];
        int len = sprintf(buffer2, "%s", edCodeString->GetString());   
        key.SetString(buffer2, static_cast<SizeType>(len), newdoc.GetAllocator());
        // Copy the desired JSON object from the source to the destination
        Value newObject(kObjectType);
        newObject.CopyFrom(props, document.GetAllocator());
        
        // add the properties object to a new object all ED and then add the new to the ouput objects
        Value secondObject(kObjectType);
        secondObject.SetObject();
        secondObject.AddMember("ED", newObject, newdoc.GetAllocator());
        Value thirdObject(kArrayType);
        secondObject.AddMember("SMA", thirdObject, newdoc.GetAllocator());
        newdoc.AddMember(key, secondObject, newdoc.GetAllocator());

    }  // end of for

    std::cout << "finished array creation " << std::endl;
    ///-----------------------------------------------//
       // now reopen file and the geojson for the smas and add

    // so in smas -> features ->properties ->"ED_ID_STR"

     // Open the file for reading 


    FILE* fps = fopen("smas2022.json", "r");
    rapidjson::Document documents;
    rapidjson::FileReadStream iss(fps, readBuffer,
        sizeof(readBuffer));
    document.ParseStream(iss);

    // Close the file 
    fclose(fps);

    char buffer3[10];
    features = rapidjson::Pointer("/features").Get(document);
    for (auto& features : features->GetArray()) {

        Value& props = features["properties"];
        rapidjson::Value  key;
        // This is my string that is the foreign key 
        Value* edCodeString = rapidjson::Pointer("/ED_ID_STR").Get(props);
        rapidjson::Document tempDocument;

        int len = sprintf(buffer3, "%s%s%s", "/", edCodeString->GetString(), "/SMA");   
        key.SetString(buffer3, static_cast<SizeType>(len), newdoc.GetAllocator());

        Value newObject(kObjectType);
        newObject.CopyFrom(features, document.GetAllocator());

        auto* smaPath = rapidjson::Pointer(key.GetString()).Get(newdoc);

        if (smaPath && smaPath->IsArray()) {
            //exists so now add the small area information
            rapidjson::Value& innerArray = newdoc[edCodeString->GetString()]["SMA"];
            innerArray.PushBack(newObject, newdoc.GetAllocator());
        }  // end of if

    } //end of array
 
    ///-----------------------------------------------//

    // NOW SAVE THE FILE - this will now write the json formed through the merge
    FILE* fpo = fopen("output_combined.json", "wb"); // non-Windows use "w"

    char writeBuffer[65536];

    FileWriteStream os(fpo, writeBuffer, sizeof(writeBuffer));

    Writer<FileWriteStream> writer(os);
    newdoc.Accept(writer);

    fclose(fpo);

    return 0;
}

