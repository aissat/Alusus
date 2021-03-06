/**
 * @file Core/Data/Grammar/StandardFactory.cpp
 * Contains the implementation of class Core::Data::Grammar::StandardFactory.
 *
 * @copyright Copyright (C) 2020 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <https://alusus.org/license.html>.
 */
//==============================================================================

#include "core.h"

namespace Core::Data::Grammar
{

using namespace Core::Data::Grammar;
using namespace Core::Processing;
using namespace Core::Processing::Handlers;

//==============================================================================
// Member Functions

/**
 * Creates the entire list of definitions for productions, tokens, and char
 * groups for the Core's grammar. This function will give you the complete
 * grammar definitions with all the required handlers.
 */
void StandardFactory::createGrammar(
  DynamicContaining<TiObject> *rootScope, Main::RootManager *root, Bool exprOnly
) {
  this->setRootScope(rootScope);

  this->constTokenPrefix = S("LexerDefs");
  this->constTokenId = ID_GENERATOR->getId(S("CONSTTOKEN"));

  // Instantiate handlers.
  this->stringLiteralHandler = newSrdObj<StringLiteralTokenizingHandler>(
    StringLiteralTokenizingHandler::OuterQuoteType::DOUBLE
  );
  this->charLiteralHandler = newSrdObj<StringLiteralTokenizingHandler>(
    StringLiteralTokenizingHandler::OuterQuoteType::SINGLE
  );
  this->constTokenHandler = newSrdObj<ConstTokenizingHandler>(this->constTokenId);
  this->identifierTokenHandler = newSrdObj<IdentifierTokenizingHandler>();
  this->parsingHandler = newSrdObj<GenericParsingHandler>();
  this->importHandler = newSrdObj<ImportParsingHandler>(root);
  this->dumpAstParsingHandler = newSrdObj<DumpAstParsingHandler>(root);
  this->leadingModifierHandler = newSrdObj<ModifierParsingHandler>(true);
  this->trailingModifierHandler = newSrdObj<ModifierParsingHandler>(false);
  this->doCommandParsingHandler = newSrdObj<GenericCommandParsingHandler>(S("do"));
  this->scopeParsingHandler = newSrdObj<ScopeParsingHandler<Data::Ast::Scope>>(root->getSeeker());
  this->rootScopeParsingHandler = newSrdObj<RootScopeParsingHandler>(root->getRootScopeHandler());

  // Create lexer definitions.
  this->set(S("root.LexerDefs"), LexerModule::create({}));
  this->createCharGroupDefinitions();
  this->createTokenDefinitions();

  // Create parser definitions.
  this->createProductionDefinitions(exprOnly);

  // Create error sync block pairs.
  this->set(S("root.ErrorSyncBlockPairs"), List::create({}, {
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S("("))}
    }),
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S(")"))}
    }),
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S("["))}
    }),
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S("]"))}
    }),
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S("{"))}
    }),
    TokenTerm::create({
      {S("tokenId"), TiInt::create(this->constTokenId)},
      {S("tokenText"), TiStr::create(S("}"))}
    })
  }));

  // Set start production and lexer module.
  Module *rootModule = this->context.getRoot();
  rootModule->setStartRef(PARSE_REF(S("module.Program")));
  rootModule->setLexerModuleRef(PARSE_REF(S("root.LexerDefs")));
  rootModule->setErrorSyncBlockPairsRef(PARSE_REF(S("root.ErrorSyncBlockPairs")));

  // Generate const token definitions from production definitions.
  this->generateConstTokenDefinitions();

  // Add predefined command keywords to identifier token handler.
  this->identifierTokenHandler->addKeywords({
    S("do"),
    S("import"), S("اشمل"),
    S("def"), S("عرّف"), S("عرف"),
    S("use"), S("استخدم"),
    S("dump_ast"), S("أدرج_ش_ب_م"),
    S("alias"), S("لقب")
  });
}


/**
 * Create a CharGroupDefinitionList and add all the required definitions for
 * the Core's grammar.
 */
void StandardFactory::createCharGroupDefinitions()
{
  // BinDigit : char '0'..'1';
  this->set(S("root.LexerDefs.BinDigit"), CharGroupDefinition::create(
    SequenceCharGroupUnit::create(S("0"), S("1"))
  ));

  // OctDigit : char '0'..'7';
  this->set(S("root.LexerDefs.OctDigit"), CharGroupDefinition::create(
    SequenceCharGroupUnit::create(S("0"), S("7"))
  ));

  // DecDigit : char '0'..'9';
  this->set(S("root.LexerDefs.DecDigit"), CharGroupDefinition::create(
    SequenceCharGroupUnit::create(S("0"), S("9"))
  ));

  // HexDigit : char '0'..'9', 'a'..'f', 'A'..'F';
  this->set(S("root.LexerDefs.HexDigit"), CharGroupDefinition::create(
    UnionCharGroupUnit::create({
      SequenceCharGroupUnit::create(S("0"), S("9")),
      SequenceCharGroupUnit::create(S("a"), S("f")),
      SequenceCharGroupUnit::create(S("A"), S("F"))
    })
  ));

  // Letter : char 'a'..'z', 'A'..'Z', '_', 0h0620..0h065F, 0h066E..0h06DC;
  SharedPtr<CharGroupUnit> letterCharGroup = UnionCharGroupUnit::create({
    SequenceCharGroupUnit::create(S("a"), S("z")),
    SequenceCharGroupUnit::create(S("A"), S("Z")),
    SequenceCharGroupUnit::create(S("_"), S("_")),
    SequenceCharGroupUnit::create(S("ؠ"), S("ٟ")),
    SequenceCharGroupUnit::create(S("ٮ"), S("ۜ"))
  });
  this->set(S("root.LexerDefs.Letter"), CharGroupDefinition::create(letterCharGroup));

  // AnyChar : char any;
  this->set(S("root.LexerDefs.AnyChar"), CharGroupDefinition::create(
    SequenceCharGroupUnit::create(WCHAR_MIN, WCHAR_MAX)
  ));

  // AnyCharNoEs : char !('\\');
  this->set(S("root.LexerDefs.AnyCharNoEs"), CharGroupDefinition::create(
    InvertCharGroupUnit::create(
      SequenceCharGroupUnit::create(S("\\"), S("\\"))
    )
  ));

  // AnyCharNoEsOrSingleQuote : char !("\\'");
  this->set(S("root.LexerDefs.AnyCharNoEsOrSingleQuote"), CharGroupDefinition::create(
    InvertCharGroupUnit::create(
      RandomCharGroupUnit::create(S("\\'"))
    )
  ));

  // AnyCharNoEsOrDoubleQuote : char !("\\\"");
  this->set(S("root.LexerDefs.AnyCharNoEsOrDoubleQuote"), CharGroupDefinition::create(
    InvertCharGroupUnit::create(
      RandomCharGroupUnit::create(S("\\\""))
    )
  ));

  // AnyCharNoReturn = !('\\');
  this->set(S("root.LexerDefs.AnyCharNoReturn"), CharGroupDefinition::create(
    InvertCharGroupUnit::create(
      SequenceCharGroupUnit::create(S("\n"), S("\n"))
    )
  ));

  // Spacing : char " \n\r\t";
  this->set(S("root.LexerDefs.Spacing"), CharGroupDefinition::create(
    RandomCharGroupUnit::create(S(" \r\n\t\u200F"))
  ));
}


/**
 * Create a TokenDefinitionList and add all the required definitions for
 * the Core's grammar.
 */
void StandardFactory::createTokenDefinitions()
{
  // Identifier : trule as { Letter + (Letter || DecDigit)*(0,endless) };
  this->set(S("root.LexerDefs.Identifier"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
         CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.Letter")) }}),
         MultiplyTerm::create({}, {
           {S("term"), AlternateTerm::create({}, {
              {S("terms"), List::create({}, {
                 CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.Letter")) }}),
                 CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})
               })}
            })}
         })
       })}
    })},
    {S("handler"), this->identifierTokenHandler}
  }));

  // IntLiteral : trule as {
  //   (DecIntLiteral || BinIntLiteral || OctIntLiteral || HexIntLiteral) +
  //   ("u" || "U")*(0,1) + (("i" || "I") + DecIntLiteral)*(0,1)
  // };
  this->set(S("root.LexerDefs.IntLiteral"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.DecIntLiteral")) }}),
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.BinIntLiteral")) }}),
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.OctIntLiteral")) }}),
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.HexIntLiteral")) }})
          })}
        }),
        MultiplyTerm::create({
          {S("max"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), AlternateTerm::create({}, {
            {S("terms"), List::create({}, {
              ConstTerm::create({{ S("matchString"), TiWStr(S("u")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("U")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("ط")) }})
            })}
          })}
        }),
        MultiplyTerm::create({
          {S("max"), newSrdObj<TiInt>(1)}
          }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              AlternateTerm::create({}, {
                {S("terms"), List::create({}, {
                  ConstTerm::create({{ S("matchString"), TiWStr(S("i")) }}),
                  ConstTerm::create({{ S("matchString"), TiWStr(S("I")) }}),
                  ConstTerm::create({{ S("matchString"), TiWStr(S("ص")) }})
                })}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.DecIntLiteral")) }})
            })}
          })}
        })
      })}
    })}
  }));

  // @inner DecIntLiteral : trule as { DecDigit*(1,endless) };
  this->set(S("root.LexerDefs.DecIntLiteral"), SymbolDefinition::create({}, {
    {S("term"), MultiplyTerm::create({
      {S("min"), newSrdObj<TiInt>(1)}
    }, {
      {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
    })}
  }));

  // @inner BinIntLiteral : trule as { ("0b" || "0B") + BinDigit*(1,endless) };
  this->set(S("root.LexerDefs.BinIntLiteral"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ConstTerm::create({{ S("matchString"), TiWStr(S("0b")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0B")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0ن")) }})
          })}
        }),
        MultiplyTerm::create({
          {S("min"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.BinDigit")) }})}
        })
      })}
    })}
  }));

  // @inner OctIntLiteral : trule as { ("0o" || "0O") + OctDigit*(1,endless) };
  this->set(S("root.LexerDefs.OctIntLiteral"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ConstTerm::create({{ S("matchString"), TiWStr(S("0o")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0O")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0م")) }})
          })}
        }),
        MultiplyTerm::create({
          {S("min"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.OctDigit")) }})}
        })
      })}
    })}
  }));

  // @inner HexIntLiteral : trule as { ("0h" || "0H") + HexDigit*(1,endless) };
  this->set(S("root.LexerDefs.HexIntLiteral"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ConstTerm::create({{ S("matchString"), TiWStr(S("0h")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0H")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0x")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("0X")) }})
          })}
        }),
        MultiplyTerm::create({
          {S("min"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }})}
        })
      })}
    })}
  }));

  // FloatLiteral : trule as {
  //   DecDigit*(1,endless) + FloatPostfix ||
  //   DecDigit*(1,endless) + FloatExponent + FloatPostfix*(0,1) ||
  //   DecDigit*(1,endless) + "." + DecDigit*(1,endless) +
  //     FloatExponent*(0,1) + FloatPostfix*(1,1)
  // };
  this->set(S("root.LexerDefs.FloatLiteral"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
  }, {
    {S("term"), AlternateTerm::create({}, {
      {S("terms"), List::create({}, {
        ConcatTerm::create({}, {
          {S("terms"), List::create({}, {
            MultiplyTerm::create({
              {S("min"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
            }),
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FloatPostfix")) }})
          })}
        }),
        ConcatTerm::create({}, {
          {S("terms"), List::create({}, {
            MultiplyTerm::create({
              {S("min"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
            }),
            ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FloatExponent")) }}),
            MultiplyTerm::create({
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FloatPostfix")) }})}
            })
          })}
        }),
        ConcatTerm::create({}, {
          {S("terms"), List::create({}, {
            MultiplyTerm::create({}, {
              {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
            }),
            ConstTerm::create({{ S("matchString"), TiWStr(S(".")) }}),
            MultiplyTerm::create({
              {S("min"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
            }),
            MultiplyTerm::create({
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FloatExponent")) }})}
            }),
            MultiplyTerm::create({
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FloatPostfix")) }})}
            })
          })}
        })
      })}
    })}
  }));

  // @inner FloatExponent : trule as { ("e" || "E") + ("+" || "-")*(0,1) + DecDigit*(1,endless) };
  this->set(S("root.LexerDefs.FloatExponent"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ConstTerm::create({{ S("matchString"), TiWStr(S("e")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("E")) }})
          })}
        }),
        MultiplyTerm::create({
          {S("max"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), AlternateTerm::create({}, {
            {S("terms"), List::create({}, {
              ConstTerm::create({{ S("matchString"), TiWStr(S("+")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("-")) }})
            })}
          })}
        }),
        MultiplyTerm::create({
          {S("min"), newSrdObj<TiInt>(1)}
        }, {
          {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
        })
      })}
    })}
  }));

  // @inner FloatPostfix : trule as { ("f" || "F") + DecDigit*(0,endless) };
  this->set(S("root.LexerDefs.FloatPostfix"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
         AlternateTerm::create({}, {
           {S("terms"), List::create({}, {
              ConstTerm::create({{ S("matchString"), TiWStr(S("f")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("F")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("ع")) }})
           })}
         }),
         MultiplyTerm::create({}, {
           {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.DecDigit")) }})}
         })
       })}
    })}
  }));

  // CharLiteral : trule as { "'" + EsCharWithDoubleQuote + "'" + CharCodePostfix*(0,1) };
  // TODO: Add the char_code_postfix part
  this->set(S("root.LexerDefs.CharLiteral"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
         ConstTerm::create({{ S("matchString"), TiWStr(S("'")) }}),
         ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.EsCharWithDoubleQuote")) }}),
         ConstTerm::create({{ S("matchString"), TiWStr(S("'")) }})
       })}
    })},
    {S("handler"), this->charLiteralHandler.s_cast<TiObject>()}
  }));

  // StringLiteral : trule as {
  //   StringLiteralPart + (Spacing*(0,endless) + StringLiteralPart)*(0,endless) +
  //   CharCodePostfix*(0,1)
  // };
  // TODO: Add the CharCodePostfix part
  this->set(S("root.LexerDefs.StringLiteral"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.StringLiteralPart")) }}),
        MultiplyTerm::create({}, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              MultiplyTerm::create({}, {
                {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.Spacing")) }})}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.StringLiteralPart")) }})
            })}
          })}
        })
      })}
    })},
    {S("handler"), this->stringLiteralHandler.s_cast<TiObject>()}
  }));

  // @inner StringLiteralPart : trule as { "\"" + EsCharWithSingleQuote*(0,endless) + "\"" };
  this->set(S("root.LexerDefs.StringLiteralPart"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ConstTerm::create({{ S("matchString"), TiWStr(S("\"")) }}),
        MultiplyTerm::create({}, {
          {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.EsCharWithSingleQuote")) }})}
        }),
        ConstTerm::create({{ S("matchString"), TiWStr(S("\"")) }})
      })}
    })}
  }));

  // @inner EsCharWithSingleQuote : trule as {
  //   AnyCharNoEsOrDoubleQuote || EsCodeSequence ||
  //   alternate (root.TokenData.escapeSequences:es)->( es )
  // };
  // TODO: Add the heap.escape_sequences part
  this->set(S("root.LexerDefs.EsCharWithSingleQuote"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("terms"), List::create({}, {
         CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.AnyCharNoEsOrDoubleQuote")) }}),
         ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.EsSequence")) }})
       })}
    })}
  }));

  // @inner EsCharWithDoubleQuote : trule as {
  //   AnyCharNoEsOrSingleQuote || EsCodeSequence ||
  //   alternate (root.TokenData.escapeSequences:es)->( es )
  // };
  // TODO: Add the root.TokenData.escapeSequences part
  this->set(S("root.LexerDefs.EsCharWithDoubleQuote"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("terms"), List::create({}, {
         CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.AnyCharNoEsOrSingleQuote")) }}),
         ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.EsSequence")) }})
       })}
    })}
  }));

  // @inner EsCharWithQuotes : trule as {
  //   AnyCharNoEs || EsCodeSequence || alternate (root.TokenData.escapeSequences:es)->( es )
  // };
  // TODO: Add the heap.escape_sequences part
  this->set(S("root.LexerDefs.EsCharWithQuotes"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("terms"), List::create({}, {
         CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.AnyCharNoEs")) }}),
         ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.EsSequence")) }})
       })}
    })}
  }));

  // @inner EsSequence : trule as {
  //   '\\' + ('c' + HexDigit*(2,2) ||
  //           'u' + HexDigit*(4,4) ||
  //           'w' + HexDigit*(8,8) ||
  //           'n' || 't' || 'r' || '"' || '\'' || '\\' || 'ج' || 'ت' || 'ر')
  // };
  this->set(S("root.LexerDefs.EsSequence"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ConstTerm::create({{ S("matchString"), TiWStr(S("\\")) }}),
        AlternateTerm::create({}, {
          {S("terms"), List::create({}, {
            ConstTerm::create({{ S("matchString"), TiWStr(S("\\")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("\"")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("'")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("n")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("t")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("f")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("r")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("ج")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("ت")) }}),
            ConstTerm::create({{ S("matchString"), TiWStr(S("ر")) }}),
            ConcatTerm::create({}, {
              {S("terms"), List::create({}, {
                AlternateTerm::create({}, {
                  {S("terms"), List::create({}, {
                    ConstTerm::create({{ S("matchString"), TiWStr(S("h")) }}),
                    ConstTerm::create({{ S("matchString"), TiWStr(S("x")) }})
                  })}
                }),
                CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }})
              })}
            }),
            ConcatTerm::create({}, {
              {S("terms"), List::create({}, {
                  ConstTerm::create({{ S("matchString"), TiWStr(S("u")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }})
              })}
            }),
            ConcatTerm::create({}, {
              {S("terms"), List::create({}, {
                  ConstTerm::create({{ S("matchString"), TiWStr(S("U")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }}),
                  CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.HexDigit")) }})
              })}
            })
          })}
        })
      })}
    })}
  }));

  //// IGNORED TOKENS

  // ignore { Spacing*(1,endless) };
  this->set(S("root.LexerDefs.IgnoredSpaces"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN|SymbolFlags::IGNORED_TOKEN)}
  }, {
    {S("term"), MultiplyTerm::create({
      {S("min"), newSrdObj<TiInt>(1)}
    }, {
      {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.Spacing")) }})}
    })}
  }));

  // @minimum ignore { "//" + any*(0,endless) + "\n" }
  // For now this is implemented as:
  // ignore { "//" + AnyCharNoReturn*(0,endless) + "\n" }
  // because the lexer still doesn't support the @minimum modifier.
  this->set(S("root.LexerDefs.LineComment"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN|SymbolFlags::IGNORED_TOKEN)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
         AlternateTerm::create({}, {
           {S("terms"), List::create({}, {
              ConstTerm::create({{ S("matchString"), TiWStr(S("//")) }}),
              ConstTerm::create({{ S("matchString"), TiWStr(S("#")) }}),
           })}
         }),
         MultiplyTerm::create({}, {
           {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.AnyCharNoReturn")) }})}
         }),
         ConstTerm::create({{ S("matchString"), TiWStr(S("\n")) }})
       })}
    })}
  }));

  // ignore { "/*" + any*(0,endless) + "*/" }
  this->set(S("root.LexerDefs.MultilineComment"), SymbolDefinition::create({
    {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN|SymbolFlags::IGNORED_TOKEN|SymbolFlags::PREFER_SHORTER)}
  }, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
         ConstTerm::create({{ S("matchString"), TiWStr(S("/*")) }}),
         MultiplyTerm::create({}, {
           {S("term"), CharGroupTerm::create({{ S("charGroupReference"), PARSE_REF(S("module.AnyChar")) }})}
         }),
         ConstTerm::create({{ S("matchString"), TiWStr(S("*/")) }})
       })}
    })}
  }));
}


/**
 * Create a Production_Definition_List and add all the required definitions for
 * the Core's grammar. The created list will have assigned parsing handlers.
 */
void StandardFactory::createProductionDefinitions(Bool exprOnly)
{
  //// Program : Main.Statements.StmtList.
  // Program : prod as Main.Statements.StmtList(DefaultMain.DefaultStatement);
  if (exprOnly) {
    this->set(S("root.Program"), SymbolDefinition::create({}, {
      {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("root.Main.Statements.Stmt")) }})},
      {S("handler"), this->parsingHandler}
    }));
  } else {
    this->set(S("root.Program"), SymbolDefinition::create({}, {
      {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("root.Main.RootStatements.StmtList")) }})},
      {S("handler"), this->parsingHandler}
    }));
  }

  this->createTokenDataModule();

  this->createStatementsProductionModule();

  this->createExpressionProductionModule();

  this->createSubjectProductionModule();

  this->createSetProductionDefinitions();

  this->createModifierProductionDefinitions();

  this->createMainProductionModule(exprOnly);
}


void StandardFactory::createTokenDataModule()
{
  this->set(S("root.TokenData"), Module::create({}));
  // assignmentOpList : keywords = ("=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "$=", "<<=", ">>=");
  this->set(S("root.TokenData.assignmentOpList"), Map::create({}, {
   {S("="), 0},
   {S("+="), 0},
   {S("-="), 0},
   {S("*="), 0},
   {S("/="), 0},
   {S("÷="), newSrdObj<TiStr>(("/="))},
   {S("%="), 0},
   {S("&="), 0},
   {S("|="), 0},
   {S("$="), 0},
   {S("<<="), 0},
   {S(">>="), 0}
  }, true));
  // comparisonOpList : keywords = ("==", "!=", "<", ">", "<=", ">=");
  this->set(S("root.TokenData.comparisonOpList"), Map::create({}, {
   {S("=="), 0},
   {S("!="), 0},
   {S("<"), 0},
   {S(">"), 0},
   {S("<="), 0},
   {S(">="), 0}
  }, true));
  // addOpList : keywords = ("+", "-");
  this->set(S("root.TokenData.addOpList"), Map::create({}, {
    {S("+"), 0},
    {S("-"), 0}
  }, true));
  // mulOpList : keywords = ("*", "/", "%");
  this->set(S("root.TokenData.mulOpList"), Map::create({}, {
    {S("*"), 0},
    {S("/"), 0},
    {S("÷"), newSrdObj<TiStr>(S("/"))},
    {S("%"), 0}
  }, true));
  // bitwiseOpList : keywords = ("|", "$", "&", "<<", ">>");
  this->set(S("root.TokenData.bitwiseOpList"), Map::create({}, {
    {S("|"), 0},
    {S("$"), 0},
    {S("&"), 0},
    {S("<<"), 0},
    {S(">>"), 0}
  }, true));
  // logOpList : keywords = ("||", "$$", "&&", "or", "nor", "xor", "xnor", "and", "nand");
  this->set(S("root.TokenData.logOpList"), Map::create({}, {
    {S("||"), 0},
    {S("$$"), 0},
    {S("&&"), 0},
    {S("or"), 0},
    {S("أو"), newSrdObj<TiStr>(S("or"))},
    {S("nor"), 0},
    {S("وليس"), newSrdObj<TiStr>(S("nor"))},
    {S("xor"), 0},
    {S("أو_حصرا"), newSrdObj<TiStr>(S("xor"))},
    {S("xnor"), 0},
    {S("وليس_حصرا"), newSrdObj<TiStr>(S("xnor"))},
    {S("and"), 0},
    {S("و"), newSrdObj<TiStr>(S("and"))},
    {S("nand"), 0},
    {S("أو_ليس"), newSrdObj<TiStr>(S("nand"))}
  }, true));
  // prefixOpList : keywords = ("++", "--", "+", "-", "!", "!!", "not");
  this->set(S("root.TokenData.prefixOpList"), Map::create({}, {
    {S("++"), 0},
    {S("--"), 0},
    {S("+"), 0},
    {S("-"), 0},
    {S("!"), 0},
    {S("!!"), 0},
    {S("not"), 0},
    {S("ليس"), newSrdObj<TiStr>(S("not"))},
    {S("..."), 0}
  }, true));
  // postfixOpList : keywords = ("++", "--");
  this->set(S("root.TokenData.postfixOpList"), Map::create({}, {
    {S("++"), 0},
    {S("--"), 0}
  }, true));
  // linkOpList : keywords = ("->", ".", ".>", "<.");
  this->set(S("root.TokenData.linkOpList"), Map::create({}, {
   {S("->"), 0},
   {S("."), 0},
   {S(".>"), 0},
   {S("<."), 0}
  }, true));
  // lowLinkOpList : keywords = ("=>", "..", "..>", "<..");
  this->set(S("root.TokenData.lowLinkOpList"), Map::create({}, {
   {S("=>"), 0},
   {S(".."), 0},
   {S("..>"), 0},
   {S("<.>"), 0}
  }, true));
  // lowerLinkOpList : keywords = (":", ":>", "<:");
  this->set(S("root.TokenData.lowerLinkOpList"), Map::create({}, {
   {S(":"), 0},
   {S(":>"), 0},
   {S("<:"), 0}
  }, true));
  // lowestLinkOpList : keywords = ("::", ::>", "<::", "in");
  this->set(S("root.TokenData.lowestLinkOpList"), Map::create({}, {
    {S("::"), 0},
    {S("::>"), 0},
    {S("<::"), 0},
    {S("in"), 0},
    {S("في"), newSrdObj<TiStr>(S("in"))}
  }, true));

  this->generateConstTokensForStrings(this->get(S("root.TokenData")));
}


void StandardFactory::createStatementsProductionModule()
{
  this->set(S("root.Statements"), Module::create({}));

  //// StmtList : Stmt { ";" Stmt }.
  // StmtList : prod (stmt:production[Stmt]) as
  //     stmt*(0, 1) + (lexer.Constant(";") + stmt*(0, 1))*(0, endless);
  this->set(S("root.Statements.StmtList"), SymbolDefinition::create({
    {S("flags"), TiInt::create(ParsingFlags::ENFORCE_PROD_OBJ)}
  }, {
    {S("term"), MultiplyTerm::create({
      {S("flags"), TiInt::create(TermFlags::ERROR_SYNC_TERM)}
    }, {
      {S("term"), ConcatTerm::create({
        {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
        {S("errorSyncPosId"), TiInt(1)}
      }, {
        {S("terms"), List::create({}, {
          MultiplyTerm::create({
            {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
            {S("max"), newSrdObj<TiInt>(1)}
          }, {
            {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.Stmt")) }})}
          }),
          MultiplyTerm::create({
            {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
            {S("max"), newSrdObj<TiInt>(1)}
          }, {
            {S("term"), TokenTerm::create({
              {S("flags"), TiInt::create(ParsingFlags::ENFORCE_TOKEN_OMIT)},
              {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
              {S("tokenText"), Map::create({}, {{S(";"),0},{S("؛"),0}})}
            })}
          })
        })}
      })}
    })},
    {S("handler"), this->scopeParsingHandler}
  }));

  //// Statement : (Variation | Variation | ...).
  this->createProdGroup(S("root.Statements.Stmt"), {});
}


void StandardFactory::createExpressionProductionModule()
{
  // Expression :
  // operand {binaryOp operand}.
  // [unaryOp] operand.
  // operand [unaryOp].
  // operand {FunctionalOp}.
  this->set(
    S("root.Expression"),
    Module::create({{S("startRef"), PARSE_REF(S("module.Exp"))}}).get()
  );

  // Exp : @single prod as LowestLinkExp + lexer.Constant("\\")*(0,1);
  this->set(S("root.Expression.Exp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowestLinkExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), newSrdObj<TiInt>(1)},
        }, {
          {S("term"), TokenTerm::create({
            {S("tokenId"), TiInt::create(this->constTokenId)},
            {S("tokenText"), TiStr::create(S("\\"))}
          })}
        })
      })}
    })},
    {S("handler"), this->parsingHandler}
  }));

  // LowestLinkExp : @single @prefix(heap.Modifiers.LowestLinkModifierCmd)
  //     prod (enable:integer=endless) as
  //     ConditionalExp + (LowestLinkOp + ConditionalExp)*(0,enable);
  this->set(S("root.Expression.LowestLinkExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ConditionalExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))},
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.lowestLinkOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ConditionalExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::LinkOperator>>(false)}
  }));

  // ConditionalExp : @single @prefix(heap.Modifiers.ConditionalModifierCmd)
  //     prod (enable:integer[0<=n<=1]=1) as
  //     ListExp + (lexer.Constant("?") + ListExp)*(0,enable);
  this->set(S("root.Expression.ConditionalExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ListExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), Map::create({}, {{S("?"), 0}, {S("؟"), 0}})}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ListExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), newSrdObj<TiInt>(1)}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::ConditionalOperator>>(false)}
  }));

  // ListExp : @single @prefix(heap.Modifiers.ListModifierCmd)
  //   prod (enable:integer=1) as
  //     @filter(enable) LowerLinkExp ||
  //                     (LowerLinkExp || lexer.Constant(",") + LowerLinkExp*(0,1)) +
  //                       (lexer.Constant(",") + LowerLinkExp*(0,1))*(0,endless);
  this->set(S("root.Expression.ListExp"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("filter"), PARSE_REF(S("args.enable"))},
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowerLinkExp")) }}),
        ConcatTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::ENFORCE_LIST_ITEM)}
        }, {
          {S("terms"), List::create({}, {
            AlternateTerm::create({}, {
              {S("terms"), List::create({}, {
                ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowerLinkExp")) }}),
                ConcatTerm::create({}, {
                  {S("terms"), List::create({}, {
                    TokenTerm::create({
                      {S("flags"), TiInt::create(ParsingFlags::ENFORCE_TOKEN_OMIT)},
                      {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                      {S("tokenText"), Map::create({}, {{S(","), 0}, {S("،"), 0}})}
                    }),
                    MultiplyTerm::create({
                      {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
                      {S("max"), newSrdObj<TiInt>(1)},
                    }, {
                      {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowerLinkExp")) }})}
                    })
                  })}
                })
              })}
            }),
            MultiplyTerm::create({
              {S("flags"), TiInt::create(
                ParsingFlags::PASS_ITEMS_UP|ParsingFlags::ENFORCE_LIST_ITEM
              )}
            }, {
              {S("term"), ConcatTerm::create({}, {
                {S("terms"), List::create({}, {
                  TokenTerm::create({
                    {S("flags"), TiInt::create(ParsingFlags::ENFORCE_TOKEN_OMIT)},
                    {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                    {S("tokenText"), Map::create({}, {{S(","), 0}, {S("،"), 0}})}
                  }),
                  MultiplyTerm::create({
                    {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
                    {S("max"), newSrdObj<TiInt>(1)}
                  }, {
                    {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowerLinkExp")) }})}
                  })
                })}
              })}
            })
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), newSrdObj<TiInt>(1)}})},
    {S("handler"), newSrdObj<ListExpParsingHandler<Ast::List>>()}
  }));

  // LowerLinkExp : @single @prefix(heap.Modifiers.LowerLinkModifierCmd)
  //     prod (enable:integer=endless) as
  //     AssignmentExp + (LowerLinkOp + AssignmentExp)*(0,enable);
  this->set(S("root.Expression.LowerLinkExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.AssignmentExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.lowerLinkOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.AssignmentExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::LinkOperator>>(false)}
  }));

  // AssignmentExp : @single @prefix(heap.Modifiers.AssignmentModifierCmd)
  //     prod (enable:integer=endless) as
  //     LogExp + (AssignmentOp + LogExp)*(0,enable);
  this->set(S("root.Expression.AssignmentExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LogExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.assignmentOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LogExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::AssignmentOperator>>(true)}
  }));

  // LogExp : @single @prefix(heap.Modifiers.LogModifierCmd)
  //     prod (enable:integer=endless) as
  //     ComparisonExp + (LogOp + ComparisonExp)*(0,enable);
  this->set(S("root.Expression.LogExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ComparisonExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.logOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ComparisonExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::LogOperator>>(false)}
  }));

  // ComparisonExp : @single @prefix(heap.Modifiers.ComparisonModifierCmd)
  //     prod (enable:integer=endless) as
  //     LowLinkExp + (ComparisonOp + LowLinkExp)*(0,enable);
  this->set(S("root.Expression.ComparisonExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowLinkExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.comparisonOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LowLinkExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::ComparisonOperator>>(false)}
  }));

  // LowLinkExp : @single @prefix(heap.Modifiers.LowLinkModifierCmd)
  //     prod (enable:integer=endless) as
  //     AddExp + (LowLinkOp + AddExp)*(0,enable);
  this->set(S("root.Expression.LowLinkExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.AddExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.lowLinkOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.AddExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::LinkOperator>>(false)}
  }));

  // AddExp : @single @prefix(heap.Modifiers.AddModifierCmd)
  //     prod (enable:integer=endless) as
  //     MulExp + (AddOp + MulExp)*(0,enable);
  this->set(S("root.Expression.AddExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.MulExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.addOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.MulExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::AdditionOperator>>(false)}
  }));

  // MulExp : @single @prefix(heap.Modifiers.MulModifierCmd)
  //     prod (enable:integer=endless) as
  //     BitwiseExp + (MulOp + BitwiseExp)*(0,enable);
  this->set(S("root.Expression.MulExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.BitwiseExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))}
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.mulOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.BitwiseExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::MultiplicationOperator>>(false)}
  }));

  // BitwiseExp : @single @prefix(heap.Modifiers.BitwiseModifierCmd)
  //     prod (enable:integer=endless) as
  //     UnaryExp + (BitwiseOp + UnaryExp)*(0,enable);
  this->set(S("root.Expression.BitwiseExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.UnaryExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable"))},
        }, {
          {S("term"), ConcatTerm::create({}, {
            {S("terms"), List::create({}, {
              TokenTerm::create({
                {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
                {S("tokenText"), PARSE_REF(S("root.TokenData.bitwiseOpList"))}
              }),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.UnaryExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("enable"), 0}})},
    {S("handler"), newSrdObj<InfixParsingHandler<Ast::BitwiseOperator>>(false)}
  }));

  // UnaryExp : @single @prefix(heap.Modifiers.UnaryModifierCmd)
  //     prod (enable1:integer[0<=n<=1]=1, enable2:integer[0<=n<=1]=1) as
  //     PrefixOp*(0,enable1) + FunctionalExp + PostfixOp*(0,enable2);
  this->set(S("root.Expression.UnaryExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable1"))}
        }, {
          {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.PrefixOp")) }})}
        }),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.FunctionalExp")) }}),
        MultiplyTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
          {S("max"), PARSE_REF(S("args.enable2"))}
        }, {
          {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.PostfixOp")) }})}
        })
      })}
    })},
    {S("vars"), Map::create({}, {
      {S("enable1"), newSrdObj<TiInt>(1)},
      {S("enable2"), newSrdObj<TiInt>(1)}
    })},
    {S("handler"), newSrdObj<OutfixParsingHandler<Ast::PrefixOperator, Ast::PostfixOperator>>()}
  }));

  // FunctionalExp : @single @prefix(heap.Modifiers.FunctionalModifierCmd)
  //     prod (operand:production[heap.Subject]=heap.Subject, fltr1:filter=null,
  //           fltr2:filter=null, dup:integer=endless) as
  //     @filter(fltr1)
  //         (operand + (@filter(fltr2) ParamPassExp || PostfixTildeExp || LinkExp(operand))*(0,dup)) ||
  //         PrefixTildeExp + operand;
  this->set(S("root.Expression.FunctionalExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.subject")) }}),
        MultiplyTerm::create({
          {S("flags"), PARSE_REF(S("args.flags"))},
          {S("max"), PARSE_REF(S("args.dup"))}
        }, {
          {S("term"), AlternateTerm::create({}, {
            {S("filter"), PARSE_REF(S("args.fltr2"))},
            {S("terms"), List::create({}, {
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.LinkExp")) }}),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.PostfixTildeExp")) }}),
              ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.ParamPassExp")) }})
            })}
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {
      {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
      {S("dup"), 0},
      {S("fltr2"), 0}
    })},
    {S("handler"), newSrdObj<ChainOpParsingHandler>()}
  }));

  // LinkExp : @single prod (operand:production[heap.Subject]=heap.Subject) as LinkOp + operand;
  this->set(S("root.Expression.LinkExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
       {S("terms"), List::create({}, {
          TokenTerm::create({
            {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
            {S("tokenText"), PARSE_REF(S("root.TokenData.linkOpList"))}
          }),
          ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.subject")) }})
        })}
     })},
    {S("handler"), newSrdObj<CustomParsingHandler>([](Parser *parser, ParserState *state) {
      auto currentList = state->getData().ti_cast_get<Containing<TiObject>>();
      auto metadata = ti_cast<Ast::MetaHaving>(currentList);
      auto token = ti_cast<Ast::Token>(currentList->getElement(0));
      auto linkOp = Ast::LinkOperator::create({
        { "prodId", metadata->getProdId() },
        { "sourceLocation", metadata->findSourceLocation() }
      });
      linkOp->setType(token->getText());
      linkOp->setSecond(getSharedPtr(currentList->getElement(1)));
      state->setData(linkOp);
    })}
  }));

  //// ParamPassExp : "(" [Expression] ")" | "[" [Expression] "]".
  // ParamPassExp : @single prod (expr:production[Expression||Statement]=heap.Expression, fltr:filter=null) as
  //     @filter(fltr) lexer.Constant("(") + expr*(0,1) + lexer.Constant(")") ||
  //                   lexer.Constant("[") + expr*(0,1) + lexer.Constant("]");
  this->set(S("root.Expression.ParamPassExp"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({
      {S("flags"), TiInt::create(ParsingFlags::ENFORCE_ROUTE_OBJ)},
    }, {
      {S("filter"), PARSE_REF(S("args.fltr"))},
      {S("terms"), List::create({}, {
        ConcatTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
        }, {
          {S("terms"), List::create({}, {
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("("))}
            }),
            MultiplyTerm::create({
              {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
              {S("max"), newSrdObj<TiInt>(1)},
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.paramPassExpr")) }})}
            }),
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S(")"))}
            })
          })}
        }),
        ConcatTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
        }, {
          {S("terms"), List::create({}, {
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("["))}
            }),
            MultiplyTerm::create({
              {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.paramPassExpr")) }})}
            }),
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("]"))}
            })
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {{S("fltr"), 0}})},
    {S("handler"), newSrdObj<CustomParsingHandler>([](Parser *parser, ParserState *state) {
      auto currentRoute = state->getData().ti_cast_get<Ast::Route>();
      auto paramPass = Ast::ParamPass::create({
        { "prodId", currentRoute->getProdId() },
        { "sourceLocation", currentRoute->findSourceLocation() }
      });
      paramPass->setType(currentRoute->getRoute()==0 ? Ast::BracketType::ROUND : Ast::BracketType::SQUARE);
      paramPass->setParam(currentRoute->getData());
      state->setData(paramPass);
    })}
  }));

  //// PostfixTildeExp :
  //// "~" keyword {Subject}.
  //// "~(" Expression {Subject} ")".
  //PostfixTildeExp : @single prod (cmd:production[PostfixTildeCmd]=DefaultPostfixTildeCmd) as
  //    lexer.Constant("~") + cmd;
  this->set(S("root.Expression.PostfixTildeExp"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        TokenTerm::create({
          {S("tokenId"), TiInt::create(this->constTokenId)},
          {S("tokenText"), TiStr::create(S("~"))}
        }),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.postfixTildeCmdGrp")) }})
      })}
    })},
    {S("handler"), this->parsingHandler}
  }));
  //OpenPostfixTildeCmd : @limit[user.parent==PostfixTildeCmd]
  //    prod (expr:production[Expression], args:list[hash[sbj:valid_subject, min:integer, max:integer]]) as
  //    lexer.Constant("(") + expr + concat (args:a)->( a.sbj*(a.min,a.max) ) +
  //    lexer.Constant(")");
  this->set(S("root.Expression.OpenPostfixTildeCmd"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({}, {
      {S("terms"), List::create({}, {
        TokenTerm::create({
          {S("tokenId"), TiInt::create(this->constTokenId)},
          {S("tokenText"), TiStr::create(S("("))}
        }),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.openPostfixTildeExpr")) }}),
        TokenTerm::create({
          {S("tokenId"), TiInt::create(this->constTokenId)},
          {S("tokenText"), TiStr::create(S(")"))}
        })
      })}
    })},
    {S("handler"), newSrdObj<CustomParsingHandler>(
      [](Parser *parser, ParserState *state)
      {
        auto data = state->getData();
        auto metadata = data.ti_cast_get<Ast::MetaHaving>();
        auto linkOp = Ast::LinkOperator::create({
          { "prodId", metadata->getProdId() },
          { "sourceLocation", metadata->findSourceLocation() }
        });
        linkOp->setType(S("~"));
        linkOp->setSecond(data);
        state->setData(linkOp);
      }
    )}
  }));

  //// Operators :
  // PrefixOp : prod ref heap.ConstantKeywordGroup(heap.TokenData.prefixOpList);
  this->set(S("root.Expression.PrefixOp"), SymbolDefinition::create({}, {
    {S("term"), TokenTerm::create({
      {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
      {S("tokenText"), PARSE_REF(S("root.TokenData.prefixOpList"))}
    })},
    {S("handler"), this->parsingHandler}
  }));
  // PostfixOp : prod ref heap.ConstantKeywordGroup(heap.TokenData.postfixOpList);
  this->set(S("root.Expression.PostfixOp"), SymbolDefinition::create({}, {
    {S("term"), TokenTerm::create({
      {S("tokenId"), newSrdObj<TiInt>(this->constTokenId)},
      {S("tokenText"), PARSE_REF(S("root.TokenData.postfixOpList"))}
    })},
    {S("handler"), this->parsingHandler}
  }));

  this->set(S("root.Expression.paramPassExpr"), TioSharedPtr::null);
  this->set(S("root.Expression.openPostfixTildeExpr"), TioSharedPtr::null);
  this->set(S("root.Expression.subject"), TioSharedPtr::null);
  this->set(S("root.Expression.postfixTildeCmdGrp"), TioSharedPtr::null);
}


void StandardFactory::createSubjectProductionModule()
{
  // Subject : Parameter | Command | Expression | Statement | Set.
  this->set(
    S("root.Subject"),
    Module::create({{S("startRef"), PARSE_REF(S("module.Sbj"))}}).get()
  );

  // Sbj : @limit[user.owner==Subject] prod (
  //   sbj1:production[Parameter||Command||Expression||Statement||Set],
  //   sbj2:production[Parameter||Command||Expression||Statement||Set],
  //   sbj3:production[Parameter||Command||Expression||Statement||Set],
  //   frc2:integer,
  //   frc3:integer,
  //   fltr:filter
  // ) as @filter(fltr) sbj1 ||
  //                    lexer.Constant("(") + sbj2*(frc2,1) + lexer.Constant(")") ||
  //                    lexer.Constant("[") + sbj3*(frc3,1) + lexer.Constant("]");
  this->set(S("root.Subject.Sbj"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("filter"), PARSE_REF(S("args.fltr"))},
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("args.sbj1")) }}),
        ConcatTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)}
        }, {
          {S("terms"), List::create({}, {
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("("))}
            }),
            MultiplyTerm::create({
              {S("min"), PARSE_REF(S("args.frc2"))},
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("args.sbj2")) }})}
            }),
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S(")"))}
            })
          })}
        }),
        ConcatTerm::create({
          {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)}
        }, {
          {S("terms"), List::create({}, {
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("["))}
            }),
            MultiplyTerm::create({
              {S("min"), PARSE_REF(S("args.frc3"))},
              {S("max"), newSrdObj<TiInt>(1)}
            }, {
              {S("term"), ReferenceTerm::create({{ S("reference"), PARSE_REF(S("args.sbj3")) }})}
            }),
            TokenTerm::create({
              {S("tokenId"), TiInt::create(this->constTokenId)},
              {S("tokenText"), TiStr::create(S("]"))}
            })
          })}
        })
      })}
    })},
    {S("vars"), Map::create({}, {
      {S("sbj1"), PARSE_REF(S("module.TermGroup"))},
      {S("sbj2"), PARSE_REF(S("module.expression"))},
      {S("sbj3"), PARSE_REF(S("module.expression"))},
      {S("frc2"), 0},
      {S("frc3"), 0},
      {S("fltr"), 0}
    })},
    {S("handler"), newSrdObj<SubjectParsingHandler>(this->constTokenId)}
  }));

  // SbjGroup: (cmdGrp || Parameter || set)
  this->createProdGroup(S("root.Subject.TermGroup"), {
    PARSE_REF(S("module.cmdGrp")),
    PARSE_REF(S("module.Parameter")),
    PARSE_REF(S("module.set"))
  });

  //// Parameter: Identifier | Literal.
  // Parameter : @single @prefix(heap.Modifiers.ParameterModifierCmd)
  //     prod (fltr:filter=null, cnsts:keywords=null) as
  //     @filter(fltr) lexer.Identifier || Literal || heap.ConstantKeywordGroup(cnsts);
  this->set(S("root.Subject.Parameter"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("filter"), PARSE_REF(S("args.fltr"))},
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.Identifier")) }}),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.Literal")) }})
      })}
    })},
    {S("vars"), Map::create({}, {{ S("fltr"), 0 }} )},
    {S("handler"), this->parsingHandler}
  }));

  // Identifier: lexer.Identifier
  this->set(S("root.Subject.Identifier"), SymbolDefinition::create({}, {
    {S("term"), TokenTerm::create({
      {S("tokenId"), newSrdObj<TiInt>(ID_GENERATOR->getId(S("LexerDefs.Identifier")))}
    })},
    {S("handler"), newSrdObj<CustomParsingHandler>([](Parser *parser, ParserState *state) {
      auto current = state->getData().ti_cast_get<Ast::Token>();
      SharedPtr<Ast::Text> newObj = newSrdObj<Ast::Identifier>();
      newObj->setValue(current->getText());
      newObj->setProdId(current->getProdId());
      newObj->setSourceLocation(current->findSourceLocation());
      state->setData(newObj);
    })}
  }));

  // Literal:
  // Literal : @single prod (fltr:filter=null) as
  //     @filter(fltr) lexer.IntLiteral || lexer.FloatLiteral || lexer.CharLiteral || lexer.StringLiteral ||
  //                   lexer.CustomLiteral;
  this->set(S("root.Subject.Literal"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("filter"), PARSE_REF(S("args.fltr"))},
      {S("terms"), List::create({}, {
        TokenTerm::create({{ S("tokenId"), TiInt::create(ID_GENERATOR->getId(S("LexerDefs.IntLiteral"))) }}),
        TokenTerm::create({{ S("tokenId"), TiInt::create(ID_GENERATOR->getId(S("LexerDefs.FloatLiteral"))) }}),
        TokenTerm::create({{ S("tokenId"), TiInt::create(ID_GENERATOR->getId(S("LexerDefs.CharLiteral"))) }}),
        TokenTerm::create({{ S("tokenId"), TiInt::create(ID_GENERATOR->getId(S("LexerDefs.StringLiteral"))) }})
      })}
    })},
    {S("vars"), Map::create({}, {{ S("fltr"), 0 }} )},
    {S("handler"), newSrdObj<CustomParsingHandler>([](Parser *parser, ParserState *state) {
      auto current = state->getData().ti_cast_get<Ast::Token>();
      SharedPtr<Ast::Text> newObj;
      if (current->getId() == ID_GENERATOR->getId(S("LexerDefs.IntLiteral"))) {
        newObj = newSrdObj<Ast::IntegerLiteral>();
      } else if (current->getId() == ID_GENERATOR->getId(S("LexerDefs.FloatLiteral"))) {
        newObj = newSrdObj<Ast::FloatLiteral>();
      } else if (current->getId() == ID_GENERATOR->getId(S("LexerDefs.CharLiteral"))) {
        newObj = newSrdObj<Ast::CharLiteral>();
      } else if (current->getId() == ID_GENERATOR->getId(S("LexerDefs.StringLiteral"))) {
        newObj = newSrdObj<Ast::StringLiteral>();
      }
      newObj->setValue(current->getText());
      newObj->setProdId(current->getProdId());
      newObj->setSourceLocation(current->findSourceLocation());
      state->setData(newObj);
    })}
  }));

  this->set(S("root.Subject.expression"), TioSharedPtr::null);
  this->set(S("root.Subject.set"), TioSharedPtr::null);
  this->set(S("root.Subject.cmdGrp"), TioSharedPtr::null);
}


void StandardFactory::createSetProductionDefinitions()
{
  // Set : "{" Statements.StmtList "}".
  // Set : @limit[child.terms==self,user.parent==self] @prefix(heap.Modifiers.DefaultModifierCmd)
  //     prod (stmt:production[Statement], ovrd:grammar_override) as
  //     lexer.Constant("{") + @override(ovrd) Statements.StmtList(stmt) + lexer.Constant("}");
  this->set(S("root.Set"), SymbolDefinition::create({}, {
    {S("term"), ConcatTerm::create({
      {S("errorSyncPosId"), TiInt(1)}
    }, {
      {S("terms"), List::create({}, {
        TokenTerm::create({
          {S("tokenId"), TiInt::create(this->constTokenId)},
          {S("tokenText"), TiStr::create(S("{"))}
        }),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("args.stmt")) }}),
        TokenTerm::create({
          {S("tokenId"), TiInt::create(this->constTokenId)},
          {S("tokenText"), TiStr::create(S("}"))}
        })
      })}
    })},
    {S("handler"), this->parsingHandler}
  }));
}


void StandardFactory::createModifierProductionDefinitions()
{
  // Modifiers
  this->set(S("root.Modifier"), Module::create({}));
  // Modifier.Expression
  this->set(S("root.Modifier.Expression"), Module::create({
    { S("startRef"), PARSE_REF(S("module.FunctionalExp")) },
    { S("baseRef"), PARSE_REF(S("root.Expression")) }
  }, {
    { S("paramPassExpr"), PARSE_REF(S("root.Main.Expression")) },
    { S("openPostfixTildeExpr"), PARSE_REF(S("root.Main.Expression")) },
    { S("subject"), PARSE_REF(S("module.owner.Subject")) }
  }));
  this->set(S("root.Modifier.Expression.FunctionalExp"), SymbolDefinition::create({
    {S("baseRef"), PARSE_REF(S("module.base.FunctionalExp")) }
  }, {
    {S("vars"), Map::create({}, {
      {S("flags"), TiInt::create(ParsingFlags::PASS_ITEMS_UP)},
      {S("dup"), TiInt::create(1)},
      {S("fltr2"), TiInt::create(2)}
     })}
  }));
  // Modifier.Subject
  this->set(S("root.Modifier.Subject"), Module::create({
    {S("baseRef"), PARSE_REF(S("root.Subject"))}
  }, {
    {S("expression"), PARSE_REF(S("root.Main.Expression"))},
    {S("set"), PARSE_REF(S("root.Main.Set"))},
    {S("cmdGrp"), PARSE_REF(S("module.owner.SubjectCmdGrp"))}
  }));
  // Modifier.CmdGroup
  this->createProdGroup(S("root.Modifier.CmdGroup"), {});
  // Modifier.SubjectCmdGrp
  this->createProdGroup(S("root.Modifier.SubjectCmdGrp"), {});
  // Modifier.Phrase
  this->set(S("root.Modifier.Phrase"), SymbolDefinition::create({}, {
    {S("term"), AlternateTerm::create({}, {
      {S("terms"), List::create({}, {
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.CmdGroup")) }}),
        ReferenceTerm::create({{ S("reference"), PARSE_REF(S("module.Expression")) }})
      })}
    })}
  }));
  // Modifier.LeadingModifier
  this->set(S("root.Modifier.LeadingModifier"), SymbolDefinition::create({
    {S("baseRef"), PARSE_REF(S("module.Phrase")) }
  }, {
    {S("handler"), this->leadingModifierHandler}
  }));
  // Modifier.TrailingModifier
  this->set(S("root.Modifier.TrailingModifier"), SymbolDefinition::create({
    {S("baseRef"), PARSE_REF(S("module.Phrase")) }
  }, {
    {S("handler"), this->trailingModifierHandler}
  }));

  // Modifiers parsing dimensions.
  this->set(S("root.LeadingModifierDim"), ParsingDimension::create({
    {S("entryTokenText"), newSrdObj<TiStr>(S("@"))},
    {S("startRef"), PARSE_REF(S("root.Modifier.LeadingModifier"))}
  }));
  this->set(S("root.TrailingModifierDim"), ParsingDimension::create({
    {S("entryTokenText"), newSrdObj<TiStr>(S("@<"))},
    {S("startRef"), PARSE_REF(S("root.Modifier.TrailingModifier"))}
  }));
}


void StandardFactory::createMainProductionModule(Bool exprOnly)
{
  this->set(S("root.Main"), Module::create({}));

  // root.Main.Statements : module inherits root.Statements {
  //   variations = (module.CmdVariation, module.ExpVariation);
  // };
  this->set(S("root.Main.Statements"), Module::create({
    {S("baseRef"), PARSE_REF(S("root.Statements"))}
  }, {
    {S("cmdGrp"), PARSE_REF(S("module.owner.LeadingCmdGrp"))},
    {S("expression"), PARSE_REF(S("module.owner.Expression"))}
  }));

  // CmdVariation : prule as (cmdGrp * (1,1));
  this->createStatementVariation(S("root.Main.Statements.CmdVariation"), {
    { PARSE_REF(S("module.cmdGrp")), TiInt::create(1), TiInt::create(1) }
  }, this->parsingHandler);

  // ExpVariation : prule as (module.expression * (1,1));
  this->createStatementVariation(S("root.Main.Statements.ExpVariation"), {
    { PARSE_REF(S("module.expression")), TiInt::create(1), TiInt::create(1) }
  }, this->parsingHandler);

  this->addProdsToGroup(S("root.Main.Statements.Stmt"), {
    PARSE_REF(S("module.CmdVariation")),
    PARSE_REF(S("module.ExpVariation"))
  });

  if (!exprOnly) {
    this->set(S("root.Main.RootStatements"), Module::create({
      {S("baseRef"), PARSE_REF(S("module.owner.Statements"))}
    }, {}));

    this->set(S("root.Main.RootStatements.StmtList"), SymbolDefinition::create({
      {S("baseRef"), PARSE_REF(S("module.base.StmtList"))}
    }, {
      {S("handler"), this->rootScopeParsingHandler}
    }));
  }

  // root.Main.Expression : module inherits root.Expression {
  //   paramPassExpr = module.owner.Expression;
  //   openPostfixTildeExpr = module.owner.Expression;
  // };
  this->set(S("root.Main.Expression"), Module::create({
    {S("baseRef"), PARSE_REF(S("root.Expression"))}
  }, {
    {S("paramPassExpr"), PARSE_REF(S("module.owner.Expression"))},
    {S("openPostfixTildeExpr"), PARSE_REF(S("module.owner.Expression"))},
    {S("subject"), PARSE_REF(S("module.owner.Subject"))},
    {S("postfixTildeCmdGrp"), PARSE_REF(S("module.owner.PostfixTildeCmdGrp"))}
  }));

  // root.Main.Subject : module inherits root.Subject {
  //   expression = module.owner.Expression;
  //   set = module.owner.Set;
  //   cmdGrp = module.owner.SubjectCmdGrp;
  // };
  this->set(S("root.Main.Subject"), Module::create({
    {S("baseRef"), PARSE_REF(S("root.Subject"))}
  }, {
    {S("expression"), PARSE_REF(S("module.owner.Expression"))},
    {S("set"), PARSE_REF(S("module.owner.Set"))},
    {S("cmdGrp"), PARSE_REF(S("module.owner.SubjectCmdGrp"))}
  }));

  // root.Main.Set
  this->set(S("root.Main.Set"), SymbolDefinition::create({
    {S("baseRef"), PARSE_REF(S("root.Set"))}
  }, {
    {S("vars"), Map::create({}, {
       {S("stmt"), PARSE_REF(S("module.Statements.StmtList"))}
     })}
  }));

  if (exprOnly) {
    // LeadingCommandGroup
    this->createProdGroup(S("root.Main.LeadingCmdGrp"), {});

    // SubjectCommandGroup
    this->createProdGroup(S("root.Main.SubjectCmdGrp"), {});

    // TildeCommandGroup
    this->createProdGroup(S("root.Main.PostfixTildeCmdGrp"), {
      PARSE_REF(S("module.Expression.OpenPostfixTildeCmd"))
    });
  } else {
    // LeadingCommandGroup
    this->createProdGroup(S("root.Main.LeadingCmdGrp"), {
      PARSE_REF(S("module.Do")),
      PARSE_REF(S("module.Import")),
      PARSE_REF(S("module.Def")),
      PARSE_REF(S("module.Use")),
      PARSE_REF(S("module.DumpAst"))
    });

    // SubjectCommandGroup
    this->createProdGroup(S("root.Main.SubjectCmdGrp"), {
      PARSE_REF(S("module.Alias"))
    });

    // TildeCommandGroup
    this->createProdGroup(S("root.Main.PostfixTildeCmdGrp"), {
      PARSE_REF(S("module.Expression.OpenPostfixTildeCmd")),
    });

    //// Do = "do" + Subject
    this->createCommand(S("root.Main.Do"), {{
      Map::create({}, { { S("do"), 0 } }),
      {{
        PARSE_REF(S("module.Subject")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, this->doCommandParsingHandler);

    //// Import = "import" + Subject
    this->createCommand(S("root.Main.Import"), {{
      Map::create({}, {{S("import"),0},{S("اشمل"),0}}),
      {{
        PARSE_REF(S("module.Expression")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, this->importHandler);

    //// Def = "def" + Subject
    this->createCommand(S("root.Main.Def"), {{
      Map::create({}, { { S("def"), 0 }, { S("عرّف"), 0 }, { S("عرف"), 0 } }),
      {{
        PARSE_REF(S("module.Expression")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, newSrdObj<DefParsingHandler>());
    this->set(S("root.Main.Def.modifierTranslations"), Map::create({}, {
      {S("دمج"), TiStr::create(S("merge"))}
    }));

    //// use = "use" + Expression
    this->createCommand(S("root.Main.Use"), {{
      Map::create({}, { { S("use"), 0 }, { S("استخدم"), 0 } }),
      {{
        PARSE_REF(S("module.Expression")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, newSrdObj<CustomParsingHandler>([](Parser *parser, ParserState *state) {
      auto metadata = state->getData().ti_cast_get<Data::Ast::MetaHaving>();
      auto currentList = state->getData().ti_cast_get<Containing<TiObject>>();
      auto bridge = Data::Ast::Bridge::create({
        {S("prodId"), metadata->getProdId()},
        {S("sourceLocation"), metadata->findSourceLocation()}
      });
      bridge->setTarget(getSharedPtr(currentList->getElement(1)));
      state->setData(bridge);
    }));

    //// dump = "dump" + Subject
    this->createCommand(S("root.Main.DumpAst"), {{
      Map::create({}, { { S("dump_ast"), 0 }, { S("أدرج_ش_ب_م"), 0 } }),
      {{
        PARSE_REF(S("module.Expression")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, this->dumpAstParsingHandler);

    //// Alias = "alias" + Subject
    this->createCommand(S("root.Main.Alias"), {{
      Map::create({}, { { S("alias"), 0 }, { S("لقب"), 0 } }),
      {{
        PARSE_REF(S("module.Expression")),
        TiInt::create(1),
        TiInt::create(1),
        TiInt::create(ParsingFlags::PASS_ITEMS_UP)
      }}
    }}, newSrdObj<CustomParsingHandler>(
      [](Parser *parser, ParserState *state)
      {
        auto currentList = state->getData().ti_cast_get<Containing<TiObject>>();
        auto metadata = ti_cast<Ast::MetaHaving>(currentList);
        auto alias = Ast::Alias::create({
          { "prodId", metadata->getProdId() },
          { "sourceLocation", metadata->findSourceLocation() }
        });
        alias->setReference(getSharedPtr(currentList->getElement(1)));
        state->setData(alias);
      }
    ));
  }
}


SharedPtr<SymbolDefinition> StandardFactory::createConstTokenDef(Char const *text)
{
  if (SBSTR(text) == S("@") || SBSTR(text) == S("@<")) {
    return SymbolDefinition::create({
      {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
    }, {
      {S("term"), ConstTerm::create({{ S("matchString"), TiWStr(text) }})}
    });
  } else {
    return SymbolDefinition::create({
      {S("flags"), TiInt::create(SymbolFlags::ROOT_TOKEN)}
    }, {
      {S("term"), ConstTerm::create({{ S("matchString"), TiWStr(text) }})},
      {S("handler"), this->constTokenHandler}
    });
  }
}

} // namespace
