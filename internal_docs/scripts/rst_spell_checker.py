'''
File Name : rst_spell_checker.py
What it does : Scans RST and RST.INC files in a directory recursively and checks for spelling errors
Version : 0.1
Requirements : Python 3.3+ and pyspellchecker (don't use pip to install, see below)
'''

'''Import standard modules'''
import os
import re

import importlib.util
import sys
import getopt

# Check if Python spell checker is installed
package_name = 'spellchecker'
spec = importlib.util.find_spec(package_name)
if spec is None:
    print(package_name +" is not installed")
    print("Run the following commands\n-----------------\n")
    print("git clone https://github.com/barrust/pyspellchecker.git")
    print("cd pyspellchecker")
    print("python setup.py install\n")
    print("Don't use pip since there are some issues with the package\n")

from spellchecker import SpellChecker

#instantiate class
spell = SpellChecker()

# Container to extract words from RST files
only_words = []
# Count of the total number of files with error
num_files_with_spelling_errors = 0
# Container with actual errors
actual_errors = []

#debug
#Container to collect all spelling errors, useful when doing for a project the first time
one_big_list = []
debug = 0

#Add words which are not in English dictionary but are valid keywords (technical words) so
#they won't show up as errors
technical_keywords = []

#Add file extensions which need to be parsed. Only REST API files are supported right now
filename_filter = {"RST", "INC", "MD"}

# Returns the filetype
def get_filetype(filename):
    # Get the last 3 or 4 letters of the filename
    # run backwards till a dot is encountered
    file_type = []

    for letter in reversed(filename):
        if letter == ".":
            break
        file_type.append(letter)

    # Didn't find a dot
    if ''.join(reversed(file_type)) == filename:
        return ''
    else:
        return ''.join(reversed(file_type))

'''
------------------Program Logic------------------
The logic is pretty simple, we recursively scan through the directory and
find files with matching extension. We open the file and read the data. Then
we run through the data to find all words with alphabets. They are first run through
spell checker and then against the array technical_keywords to find out spelling errors

If you think a valid technical keyword is being shown as spelling error then add it to the 
technical_keywords array and also commit it to GIT so others are not doing this again and again

All errors are printed to standard output
-------------------------------------------------'''

PATH="."  #Current directory
try:
  opts, args = getopt.getopt(sys.argv[1:],"hp:",["path="])
except getopt.GetoptError:
    print ("rst_spell_checker.py -p <PATH_TO_DIRECTORY>")
    sys.exit(2)
for opt, arg in opts:
    if opt == '-h':
        print ("rst_spell_checker.py -p <PATH_TO_DIRECTORY>")
        sys.exit()
    elif opt in ("-p", "--path"):
        PATH = arg

'''First populate the technical keywords by reading them from a file.
All technical keywords are kept in a file called keywords.txt in the same directory
so multiple can change it without touching the main script'''
keywords_file = open("keywords.txt", "r", errors='ignore')
#Read all lines at one go
keywords_buffer = keywords_file.readlines()
# Close the file
keywords_file.close()
for each_keyword in keywords_buffer:
    technical_keywords.append(each_keyword.rstrip())

'''Now traverse the documentation directory and find spelling errors'''

for path, dirs, files in os.walk(PATH): #Walk through all sub directories
    for filename in files:
        fullpath = os.path.join(path, filename)
        file_type = get_filetype(filename)
        # compare in list to see approved list of files.
        if file_type.lower() in (element.lower() for element in filename_filter):     
            #Open file
            rst_file = open(fullpath, "r", errors='ignore')
            #print("Opening file : ", fullpath)
            #Read all lines at one go
            buffer = rst_file.readlines()
            # Close the file
            rst_file.close()

            #Clear out the array which is used to hold extracted words
            only_words = []

            # Process all lines in file
            for lines in buffer:
                #Extract words
                words = lines.split()
                for word in words:
                    #Extract only alphanumeric words
                    #if re.match('^[a-zA-Z0-9_]+$', word):  #Alphanumeric filter
                    if re.match('^[a-zA-Z]+$', word):
                        only_words.append(word)  #Create a list of english words

            #This creates an array of all mis-spelt words
            misspelled = spell.unknown(only_words)
            #Now we run through the mis-spelt array and find out which ones are not part
            #of technical keyword
            spelling_error = 0
            actual_errors = []
            for mis_spelt_word in misspelled:
                if not mis_spelt_word.lower() in (element.lower() for element in technical_keywords):
                    spelling_error = spelling_error + 1
                    actual_errors.append(mis_spelt_word)
                    if(debug):
                        one_big_list.append(mis_spelt_word)

            # If there are errors then show to user
            if spelling_error:
                num_files_with_spelling_errors = num_files_with_spelling_errors + 1 
                print("File : ", fullpath, "has ", spelling_error, " spelling errors")
                print(actual_errors)
                print("\n")
                
    
print("Found ", num_files_with_spelling_errors, "files with spelling errors\n")
#remove all duplicate entries from the big list with all spelling errors
if(debug): 
    unique_list = list(set(one_big_list))
    for entries in unique_list:
        print(entries)

       