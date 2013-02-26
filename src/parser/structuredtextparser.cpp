#include "structuredtextparser.h"

#include <cstdio>
#include <iostream>
#include <utility>

#include <QDebug>

#include "yyheader.h"
#include "db.tab.h"

using std::cout;
using std::endl;

extern struct yycontext yyctx;
extern int yyparse (void);

int StructuredTextDataStore::dumpIndent = 0;

StructuredTextDataStore::StructuredTextDataStore()
{
  m_mode = KEY_VALUE;
  m_currentBlock = NULL;
}

void StructuredTextDataStore::put(string key, string value)
{
  switch (m_mode) {
  case KEY_VALUE:
    m_valueData[key] = value;
    break;
  case BLOCK:
    m_currentBlock->put(key, value);
    break;
  }
}

void StructuredTextDataStore::newElement(string name)
{
  if (m_currentBlock != NULL) { // nested block
    m_currentBlock->newElement(name);
  } else {
    m_mode = BLOCK;
    m_currentElementName = name;
    m_currentBlock = new StructuredTextDataStore;
  }
}

bool StructuredTextDataStore::commitElement(void)
{
  if (!m_currentBlock) {
    return false;
  }

  if (m_currentBlock != NULL) {
    if (!m_currentBlock->commitElement()) {
      m_mode = KEY_VALUE;
      m_blockData.insert(std::make_pair(m_currentElementName, m_currentBlock));
      m_currentBlock = NULL;
      return true;
    }
  }
  return true;
}

string StructuredTextDataStore::get(string key)
{
  if (m_valueData.find(key) != m_valueData.end()) {
    return m_valueData[key];
  }
  throw InvalidKeyException(key.c_str());
}

StructuredTextDataStore::BlockIterPair
StructuredTextDataStore::getBlocksByKey(string key)
{
  return m_blockData.equal_range(key);
}

const StructuredTextDataStore::ValueType&
StructuredTextDataStore::getValueData(void)
{
  return m_valueData;
}

void StructuredTextDataStore::dump(void)
{
  for (ValueType::const_iterator iter = m_valueData.begin();
      iter != m_valueData.end(); ++iter) {
    cout << string(dumpIndent * 2, ' ');
    cout << iter->first << " = " << iter->second << endl;
  }

  dumpIndent += 1;
  for (BlockType::const_iterator iter = m_blockData.begin();
      iter != m_blockData.end(); ++iter) {
    cout << string(dumpIndent * 2 - 2, ' ');
    cout << iter->first << " {" << endl;
    iter->second->dump();
    cout << string(dumpIndent * 2 - 2, ' ');
    cout << "}" << endl;
  }
  dumpIndent -= 1;
}

StructuredTextParser::StructuredTextParser(const QString& filename):
  Parser(filename)
{
}
 
StructuredTextParser::~StructuredTextParser()
{
}

StructuredTextDataStore* StructuredTextParser::parse(void)
{
  FILE* fin = freopen(m_fileName.toAscii(), "r", stdin);
  if (fin == NULL) {
    qDebug("parse: can't open `%s' for reading", qPrintable(m_fileName));
  }

  yyctx.stds = new StructuredTextDataStore;
  yyparse();
  //data->dump();

  return yyctx.stds;
}
