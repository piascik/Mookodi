This is the SAAO toy with the pipeline functions added as two new controllers
ReductionController.py and AcquisitionController.py.

Documentation for this is in GoogleDocs:
https://docs.google.com/document/d/1zENmg9zXaqkaq38EW7FjdhcPZ7ZIGpRrHMMqA9d4-AU


The functions can be tested using test.py but you need to generate the test FITS
first. The python script in testdata will create a set of test data for you and then 
you can run test.py. E.g., 
 cd testdata
 ./generate_testdata.py
 cd ..
 ./test.py


