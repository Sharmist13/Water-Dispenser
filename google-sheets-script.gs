// Automatic Water Dispenser - Google Script code
// https://github.com/StorageB/Water-Dispenser
//
// Google Sheets logging setup:
// https://github.com/StorageB/Google-Sheets-Logging
//
// This code uses the HTTPSRedirect library by Sujay Phadke
// https://github.com/electronicsguy/ESP8266
 

// Enter Spreadsheet ID here
var SS = SpreadsheetApp.openById('enter spreadsheet ID here');

var sheet = SS.getSheetByName('Sheet1');        // creates sheet class for Sheet1
var sheet2 = SS.getSheetByName('Calculations'); // creates sheet class for Calculations sheet
var str = "";

function doPost(e) {

  var parsedData;
  var result = {};
  
  try { 
    parsedData = JSON.parse(e.postData.contents);
  } 
  catch(f){
    return ContentService.createTextOutput("Error in parsing request body: " + f.message);
  }
   
  if (parsedData !== undefined){
    var flag = parsedData.format; 
    if (flag === undefined){
      flag = 0;
    }
    
    var dataArr = parsedData.values.split(","); // creates an array of the values taken from Arduino code
    
    var date_now = Utilities.formatDate(new Date(), "CST", "yyyy/MM/dd"); // gets the current date
    var time_now = Utilities.formatDate(new Date(), "CST", "hh:mm a");    // gets the current time
    
    var value0 = dataArr [0]; // run_total variable from Arduino code
    
    
    // read and execute command from the "payload_base" string from Arduino code
    switch (parsedData.command) {
      
      case "insert_row":
         
         //sheet.insertRows(2); // insert full row directly below header text
         
         var range = sheet.getRange("A2:D2");
         range.insertCells(SpreadsheetApp.Dimension.ROWS); // insert cells just above the existing data instead of inserting an entire row
         
         sheet.getRange('A2').setValue(date_now);  // publish current date into Sheet1 cell A2
         sheet2.getRange('B3').setValue(date_now); // publish current date into Calculations sheet cell B3
         sheet.getRange('B2').setValue(time_now);  // publish current time into Sheet1 cell B2 
         sheet.getRange('C2').setValue(value0);    // publish run_total to Sheet1 cell C2
         
         var ounces  = ((sheet.getRange('C2').getValue() ) * sheet2.getRange('B1').getValue() ) / 1000 * 128; // calculate how many ounces used based on the conversion factor and run time
         sheet.getRange('D2').setValue(ounces);   // publish ounces used to Sheet1 cell D2
        
         sheet.getRange('G9:G10').setValue(sheet2.getRange('G4').getValue() + " oz"); // publish value calculated from Calculations sheet cell G4
         
         str = "Data published"; // string to return back to serial console
         SpreadsheetApp.flush();
         break;     
       
    }
    
    return ContentService.createTextOutput(str);
  } // endif (parsedData !== undefined)
  
  else{
    return ContentService.createTextOutput("Error! Request body empty or in incorrect format.");
  }
    
}

