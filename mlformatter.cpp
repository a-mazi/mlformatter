/* Copyright © Artur Maziarek MMXX
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <ostream>
#include <vector>
#include <regex>
#include <map>


struct BreakTrigger
{
  enum class Position
  {
    both,
    after,
  };

  std::string pattern;
  Position position;
  bool isExclusive;
};

struct StringCell
{
  std::string text;
  bool isExcluded;
};

enum class Format
{
  any,
  html,
  fodt,
  xml,
};


const char* newLine {"\n"};

const std::map<Format, std::vector<BreakTrigger>> lineBreakTriggersSet {{
  {Format::any, {
    {"([^\\.]*\\.\\s+)([A-Z]+)", BreakTrigger::Position::after, true},
    {       "(\\?\\s+)([A-Z]+)", BreakTrigger::Position::after, true},
    {         "(!\\s+)([A-Z]+)", BreakTrigger::Position::after, true},
  }},
  {Format::html, {
    {         "(<br/>)"        , BreakTrigger::Position::after, true},
    {       "(<[^>]*>)"        , BreakTrigger::Position::both , true},
    {             "(})"        , BreakTrigger::Position::after, true},
  }},
}};

const std::map<Format, std::vector<std::string>> breakTriggerAntiPatternsSet {{
  {Format::html, {
    "</?b>", "</?i>", "</?y>", "</?strike>", "</?tt>", "</?code>",
    "</?sup>", "</?sub>", "</?ins>", "</?del>", "</?big>", "</?small>",
    "</?span[^>]*>",
    "ul\\.", "pl\\.",
  }},
  {Format::fodt, {
    "</?text:span[^>]*>",
  }},
}};

const std::map<Format, std::vector<std::vector<std::string>>> finalReplacesSet {{
  {Format::html, {
    {"<meta name=\"generator\".*>",""},
    {"<meta name=\"created\".*>",""},
    {"<meta name=\"changed\".*>",""},
    {"h\\d.+\\{.+\\}",""},
    {"(pre|code|kbd|tt).+\\{.+\\}",""},
    {"(page \\{.*) size: [a-z0-9\\.\\s]+;","$1"},
    {"a\\.?[a-z]*:link\\s*\\{.*\\}",""},
    {"a\\.?[a-z]*:visited\\s*\\{.*\\}",""},
    {"lang=\"\\w+-\\w+\"",""}, {"dir=\"ltr\"",""},
    {"margin-top: 0cm",""}, {"margin-bottom: 0cm",""}, {"line-height: 100%",""},
    {"(p \\{.*) background: [a-z]+;","$1"},
    {"(p \\{.*); background: [a-z]+","$1"},
    {"style=\"background: #?[\\w\\d]+\"\\s",""},
    {"style=\"[\\s;]*\"",""}, {"class=\"western\"",""},
    {"(<body).*>","$1>"},
    {"(</?[\\d\\w]+)\\s*>","$1>"},
    {"<\\s*<","<"},
  }},
  {Format::any, {
    {"\\s+\\n","\n"},
    {"[\\u0009\\u0020]+"," "},
  }},
}};

std::vector<std::string> noTouchDelimiters;

std::vector<Format> lineBreakTriggerFormats;
std::vector<Format> lineBreakAntiPatternsFormats;
std::vector<Format> finalReplacerFormats;
using Processor = std::function<void(std::string&)>;
std::vector<Processor> processors;
std::vector<Processor> subProcessors;

const char* dirDelimiter = "\\/";
std::string workingDir;

int headerLevelDownShift = 0;
static constexpr int asbMaxHeaderLevel = 7;
int maxHeaderLevel = asbMaxHeaderLevel;
int headerNumber[asbMaxHeaderLevel]{};

// File operations
bool detectFormat (const std::string& fileName);
void readFromFile(const std::string& filePath, std::string& text);
void writeToFile(const std::string& filePath, std::string& text);

// Processors
void superProcessorAroundNoTouch(std::string& text);
void newLineEreaser(std::string& text);
void lineBreaker(std::string& text);
void finalReplacer(std::string& text);
void htmlPictureEmbedder(std::string& text);
void htmlTitleFromH1(std::string& text);
void htmlHeaderNumberer(std::string& text);

// Helper functions
std::vector<StringCell> detectTags (std::string& text, const BreakTrigger& breakTrigger);
void base64Encode(char* inputData, size_t inputDataSize, std::stringstream& outputData);


int main (int argc, char * argv[]) {

  std::string filePath;

  if (argc > 1)
  {
    filePath = argv[1];
    std::smatch match;
    std::stringstream regexString;
    regexString << "[^" << dirDelimiter << "]*$";
    std::regex_search(filePath, match, std::regex{regexString.str()});
    workingDir = match.prefix();
  } else
  {
    return 0;
  }

  if (!detectFormat(filePath)) return 0;

  std::string text;

  readFromFile(filePath, text);

  for (auto processor : processors)
  {
    processor(text);
  }

  writeToFile(filePath, text);

  return 0;
}

// File operation functions

bool detectFormat (const std::string& fileName)
{
  bool isFormatDetected{false};
  std::smatch match;
  std::regex_search(fileName, match, std::regex{"\\.([^\\.]*)$"});
  std::string fileExtension = match[1].str();

  if(std::regex_match(fileExtension, std::regex("htm[l]?")))
  {
    subProcessors.push_back(newLineEreaser);
    subProcessors.push_back(htmlTitleFromH1);
    subProcessors.push_back(htmlHeaderNumberer);
    subProcessors.push_back(lineBreaker);
    lineBreakTriggerFormats.push_back(Format::html);
    lineBreakTriggerFormats.push_back(Format::any);
    lineBreakAntiPatternsFormats.push_back(Format::html);
    subProcessors.push_back(finalReplacer);
    finalReplacerFormats.push_back(Format::html);
    finalReplacerFormats.push_back(Format::any);
    subProcessors.push_back(htmlPictureEmbedder);
    noTouchDelimiters = {"<pre[^>]*>","</[^>]+>"};
    processors.push_back(superProcessorAroundNoTouch);
    isFormatDetected = true;
  }
  else if (fileExtension == "fodt")
  {
    processors.push_back(newLineEreaser);
    processors.push_back(lineBreaker);
    lineBreakTriggerFormats.push_back(Format::any);
    lineBreakAntiPatternsFormats.push_back(Format::fodt);
    processors.push_back(finalReplacer);
    finalReplacerFormats.push_back(Format::any);
    isFormatDetected = true;
  }
  else if (fileExtension == "xml")
  {
    processors.push_back(newLineEreaser);
    processors.push_back(lineBreaker);
    lineBreakTriggerFormats.push_back(Format::any);
    processors.push_back(finalReplacer);
    finalReplacerFormats.push_back(Format::any);
    isFormatDetected = true;
  }

  return isFormatDetected;
}

void readFromFile(const std::string& filePath, std::string& text)
{
  std::fstream file;
  file.open(filePath, std::ios::in);
  if (file.good())
  {
    file.seekg(0, file.end);
    size_t fileLength = file.tellg();
    file.seekg(0, file.beg);

    char* inputCString = new char[fileLength + 1];
    file.read(inputCString, fileLength);
    inputCString[fileLength] = '\0';

    text = inputCString;

    delete[] inputCString;
  }
  file.close();
}

void writeToFile(const std::string& filePath, std::string& text)
{
  std::fstream file;
  file.open(filePath, std::ios::out);
  file << text;
  file.close();
}

// Processor implementations

void superProcessorAroundNoTouch(std::string& text)
{
  std::smatch match;
  if(std::regex_search(text, match, std::regex{noTouchDelimiters[0]}))
  {
    std::string prefix            = match.prefix().str();
    std::string openingTag        = match[0].str();
    std::string noTouchWithSuffix = match.suffix().str();

    for (auto processor : subProcessors)
    {
      processor(prefix);
    }

    std::regex_search(noTouchWithSuffix, match, std::regex{noTouchDelimiters[1]});

    std::string noTouch    = match.prefix().str();
    std::string closingTag = match[0].str();
    std::string suffix     = match.suffix().str();

    superProcessorAroundNoTouch(suffix);

    std::stringstream newText;
    newText << prefix << openingTag << newLine << noTouch << newLine << closingTag << newLine << suffix;
    text = newText.str();
  }
  else
  {
    for (auto processor : subProcessors)
    {
      processor(text);
    }
  }
}

void newLineEreaser(std::string& text)
{
  text = std::regex_replace(text, std::regex{"[\\r\\n]+"}, " ");
}

void lineBreaker(std::string& text)
{
  std::vector<StringCell> taggedString;
  taggedString.push_back(StringCell{text, false});

  for (auto format : lineBreakTriggerFormats)
  {
    const auto& breakTriggers = lineBreakTriggersSet.at(format);
    for (auto breakTrigger : breakTriggers)
    {
      std::vector<StringCell> newTaggedString;
      for (auto taggedStringCell : taggedString)
      {
        if (!taggedStringCell.isExcluded)
        {
          std::vector<StringCell> detectedTaggedString =
            detectTags(taggedStringCell.text, breakTrigger);

          if (detectedTaggedString.size() != 0)
          {
            newTaggedString.insert(newTaggedString.end(),
                                   detectedTaggedString.begin(),
                                   detectedTaggedString.end());
          }
        }
        else
        {
          newTaggedString.push_back(std::move(taggedStringCell));
        }
      }
      taggedString = std::move(newTaggedString);
    }
  }

  if (taggedString[0].text == newLine)
  {
    taggedString.erase(taggedString.begin());
  }

  bool isPrevoiusANewLine{false};
  std::stringstream outputStream;

  for (auto taggedStringCell : taggedString)
  {
    if (!std::regex_match(taggedStringCell.text, std::regex("^[^\\S\\r\\n]+$")))
    {
      if (taggedStringCell.text == newLine)
      {
        if (!isPrevoiusANewLine)
        {
          outputStream << taggedStringCell.text;
        }
        isPrevoiusANewLine = true;
      }
      else
      {
        if (isPrevoiusANewLine)
        {
          taggedStringCell.text = std::regex_replace(taggedStringCell.text, std::regex{"^[^\\S\\r\\n]+"}, "");
        }
        outputStream << taggedStringCell.text;
        isPrevoiusANewLine = false;
      }
    }
  }
  text = outputStream.str();
}

void finalReplacer(std::string& text)
{
  for (auto format : finalReplacerFormats)
  {
    const auto& finalReplaces = finalReplacesSet.at(format);
    for (auto finalReplace : finalReplaces)
    {
      text = std::regex_replace(text, std::regex{finalReplace[0]}, finalReplace[1]);
    }
  }
}

// Helper function implementations

std::vector<StringCell> detectTags (std::string& text, const BreakTrigger& breakTrigger)
{
  std::vector<StringCell> taggedString;
  std::smatch match;

  while (std::regex_search(text, match, std::regex{breakTrigger.pattern}))
  {
    std::string prefix    = match.prefix().str();
    std::string tag       = match[1].str();
    std::string tagSuffix = match[2].str();
    std::string suffix    = tagSuffix + match.suffix().str();

    bool doLineBreak = true;
    for (auto format : lineBreakAntiPatternsFormats)
    {
      const auto& breakTriggerAntiPatterns = breakTriggerAntiPatternsSet.at(format);
      for (auto triggerAntiPattern : breakTriggerAntiPatterns)
      {
        if (std::regex_search(tag, std::regex(triggerAntiPattern)))
        {
          doLineBreak = false;
        }
      }
    }

    if (!prefix.empty())
    {
      taggedString.push_back(StringCell{std::move(prefix), false});
    }

    if (doLineBreak && (breakTrigger.position == BreakTrigger::Position::both))
    {
      taggedString.push_back(StringCell{newLine, true});
    }

    taggedString.push_back(StringCell{std::move(tag), breakTrigger.isExclusive});

    if (doLineBreak && ((breakTrigger.position == BreakTrigger::Position::both) ||
                        (breakTrigger.position == BreakTrigger::Position::after)))
    {
      taggedString.push_back(StringCell{newLine, true});
    }
    text = std::move(suffix);
  }

  if (!text.empty())
  {
    taggedString.push_back(StringCell{std::move(text), false});
  }

  return taggedString;
}

void htmlPictureEmbedder(std::string& text)
{
  std::stringstream istream{text};
  std::stringstream ostream;
  std::string tag;
  char newLineChar = newLine[0];

  while (std::getline(istream, tag, newLineChar))
  {
    std::smatch match;
    if (std::regex_search(tag, match, std::regex{"(<img src=\")([^\"]+\\.(\\w+))(\"[^>]*>)"}))
    {
      auto prefix          = match[1].str();
      auto pictureFileName = match[2].str();
      auto pictureFileExt  = match[3].str();
      auto suffix          = match[4].str();

      ostream << match.prefix()
              << prefix
              << "data:image/"
              << pictureFileExt
              << ";base64,\n";

      std::string pictureFilePath = workingDir + pictureFileName;
      std::ifstream pictureFile;
      pictureFile.open(pictureFilePath, std::ifstream::binary);

      if (pictureFile.good())
      {
        pictureFile.seekg(0, pictureFile.end);
        size_t fileLength = pictureFile.tellg();
        pictureFile.seekg(0, pictureFile.beg);

        char* readBuf = new char[fileLength];
        pictureFile.read(readBuf, fileLength);

        if (pictureFile.good())
        {
          base64Encode(readBuf, fileLength, ostream);
        }
        delete[] readBuf;
      }
      pictureFile.close();
      std::remove(pictureFilePath.c_str());

      ostream << "\n"
              << suffix
              << match.suffix()
              << newLineChar;
    }
    else
    {
      ostream << tag << newLineChar;
    }
  }
  text = ostream.str();
}

void htmlTitleFromH1(std::string& text)
{
  std::smatch match;
  std::regex_search(text, match, std::regex{"<h1[^>]*>([^<]*)</h1>"});
  std::stringstream newTitle;
  newTitle << "<title>" << match[1].str() << "</title>";
  text = std::regex_replace(text, std::regex{"<title>[^<]*</title>"}, newTitle.str());
}

void htmlHeaderNumberer(std::string& text)
{
  std::stringstream istream{text};
  std::stringstream ostream;
  std::string tag;
  static constexpr char tagOpenBracket = '<';

  while (std::getline(istream, tag, tagOpenBracket))
  {
    std::smatch match;
    if (std::regex_search(tag, match, std::regex{"(^h(\\d)[^>]*>[\\s]*)([\\d\\.]*)"}))
    {
      auto prefix        = match[1].str();
      auto headerLevel   = std::stoi(match[2].str());
      auto currentNumber = match[3].str();

      if (currentNumber.empty())
      {
        headerLevelDownShift = headerLevel;
        maxHeaderLevel = asbMaxHeaderLevel - headerLevelDownShift;

        ostream << tagOpenBracket << tag;
        continue;
      }

      headerLevel -= headerLevelDownShift;

      if ((headerLevel > 0) && (headerLevel <= maxHeaderLevel))
      {
        ostream << tagOpenBracket
                << match.prefix()
                << prefix;

        headerNumber[headerLevel-1]++;

        for (int level = 1; level <= headerLevel; level++)
        {
          ostream << headerNumber[level-1] << ".";
        }

        for (int level = headerLevel + 1; level <= maxHeaderLevel; level++)
        {
          headerNumber[level-1] = 0;
        }

        ostream << match.suffix();
      }
      else
      {
        ostream << tagOpenBracket << tag;
      }
    }
    else
    {
      ostream << tagOpenBracket << tag;
    }
  }
  text = ostream.str();
}


void base64Encode(char* inputData, size_t inputDataSize, std::stringstream& outputData)
{
  static constexpr size_t inputDataChunkSize  =  3; // bytes
  static constexpr size_t outputDataChunkSize =  4; // bytes
  static constexpr size_t inputDataByteSize   =  6; // bits
  static constexpr uint32_t base64CharIndexMask = (1 << inputDataByteSize) - 1;
  static constexpr size_t bitsizeof_uint32_t  = 32; // bits
  static constexpr size_t bitsizeof_char      =  8; // bits
  static constexpr size_t encodedLineLength   = outputDataChunkSize * 19;
  char outputDataChunk[outputDataChunkSize+1];
  outputDataChunk[outputDataChunkSize] = 0x00;
  const char* base64CharSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "0123456789+/";
  char lastBase64Char;

  size_t inputDataIndex  = 0;
  size_t outputDataIndex = 0;

  for (; inputDataIndex < (inputDataSize - inputDataChunkSize);)
  {
    uint32_t inputDataChunk = 0;
    for (size_t inputDataChunkIndex = 0; inputDataChunkIndex < inputDataChunkSize; inputDataChunkIndex++)
    {
      uint32_t inputDataChunkShift = bitsizeof_uint32_t - ((inputDataChunkIndex + 1) * bitsizeof_char);
      uint8_t inputChar = inputData[inputDataIndex + inputDataChunkIndex];
      inputDataChunk |= inputChar << inputDataChunkShift;
    }
    for (size_t outputDataChunkIndex = 0; outputDataChunkIndex < outputDataChunkSize; outputDataChunkIndex++)
    {
      uint32_t base64CharIndexShift = bitsizeof_uint32_t - ((outputDataChunkIndex + 1) * inputDataByteSize);
      uint32_t base64CharIndex = (inputDataChunk >> base64CharIndexShift) & base64CharIndexMask;
      lastBase64Char = outputDataChunk[outputDataChunkIndex] = base64CharSet[base64CharIndex];
    }
    outputData << outputDataChunk;
    inputDataIndex  += inputDataChunkSize;
    outputDataIndex += outputDataChunkSize;
    if ((outputDataIndex % encodedLineLength) == 0)
    {
      outputData << std::endl;
    }
  }

  uint32_t inputDataChunk = 0;
  uint32_t lastInputDataChunkSize = inputDataSize - inputDataIndex;
  for (size_t inputDataChunkIndex = 0; inputDataChunkIndex < lastInputDataChunkSize; inputDataChunkIndex++)
  {
    uint32_t inputDataChunkShift = bitsizeof_uint32_t - ((inputDataChunkIndex + 1) * bitsizeof_char);
    uint8_t inputChar = inputData[inputDataIndex + inputDataChunkIndex];
    inputDataChunk |= inputChar << inputDataChunkShift;
  }
  uint32_t lastOutputDataChunkSizeBits = 0;
  for (size_t outputDataChunkIndex = 0; outputDataChunkIndex < outputDataChunkSize; outputDataChunkIndex++)
  {
    if (lastOutputDataChunkSizeBits < (lastInputDataChunkSize * bitsizeof_char))
    {
      uint32_t base64CharIndexShift = bitsizeof_uint32_t - ((outputDataChunkIndex + 1) * inputDataByteSize);
      uint32_t base64CharIndex = (inputDataChunk >> base64CharIndexShift) & base64CharIndexMask;
      lastBase64Char = outputDataChunk[outputDataChunkIndex] = base64CharSet[base64CharIndex];
    }
    else
    {
      outputDataChunk[outputDataChunkIndex] = lastBase64Char;
    }
    lastOutputDataChunkSizeBits += inputDataByteSize;
  }

  outputData << outputDataChunk;
}

/* To use it in git hooks modify the following file:
 * .git/hooks/pre-commit
 *
 * with this content:

#!/bin/sh
git diff --cached --name-only --diff-filter=ACM | xargs -n 1 -I '{}' ../tools/mlformatter '{}'
git diff --cached --name-only --diff-filter=ACM | xargs -n 1 -I '{}' git add '{}'

 * Alternatively a gitconfig can be used like this:

[filter "formatter"]
    smudge = tools/mlformatter
    clean = tools/mlformatter

 */

/* To setup LibreWriter to run the program after each file write:
 * Menu: Tools -> Customize -> Events
 * Save in: LibreOffice
 * Event: Document has been saved
 * Assigned Action: Standard.Module1.Main

Sub Main
Dim url As String
url = ConvertFromURL( ThisComponent.Url )
Shell "/home/user/mlformatter " & url
End Sub

 */
