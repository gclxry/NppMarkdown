
/*
	Copyright (c) 2009 by Chad Nelson
	Released under the MIT License.
	See the provided LICENSE.TXT file for details.
*/
#include "markdown-tokens.h"

#include <stack>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_set.hpp>

using std::cerr;
using std::endl;

namespace markdown {
namespace token {

namespace {

const std::string cEscapedCharacters("\\`*_{}[]()#+-.!>");

optional<size_t> isEscapedCharacter(char c) {
	std::string::const_iterator i=std::find(cEscapedCharacters.begin(),
		cEscapedCharacters.end(), c);
	if (i!=cEscapedCharacters.end())
		return std::distance(cEscapedCharacters.begin(), i);
	else return none;
}

char escapedCharacter(size_t index) {
	return cEscapedCharacters[index];
}

std::string encodeString(const std::string& src, int encodingFlags) {
	bool amps=(encodingFlags & cAmps)!=0,
		doubleAmps=(encodingFlags & cDoubleAmps)!=0,
		angleBrackets=(encodingFlags & cAngles)!=0,
		quotes=(encodingFlags & cQuotes)!=0;

	std::string tgt;
	for (std::string::const_iterator i=src.begin(), ie=src.end(); i!=ie; ++i) {
		if (*i=='&' && amps) {
			static const boost::regex cIgnore("^(&amp;)|(&#[0-9]{1,3};)|(&#[xX][0-9a-fA-F]{1,2};)");
			if (boost::regex_search(i, ie, cIgnore)) {
				tgt.push_back(*i);
			} else {
				tgt+="&amp;";
			}
		}
		else if (*i=='&' && doubleAmps) tgt+="&amp;";
		else if (*i=='<' && angleBrackets) tgt+="&lt;";
		else if (*i=='>' && angleBrackets) tgt+="&gt;";
		else if (*i=='\"' && quotes) tgt+="&quot;";
		else tgt.push_back(*i);
	}
	return tgt;
}

bool looksLikeUrl(const std::string& str) {
	const char *schemes[]={ "http://", "https://", "ftp://", "ftps://",
		"file://", "www.", "ftp.", 0 };
	for (size_t x=0; schemes[x]!=0; ++x) {
		const char *s=str.c_str(), *t=schemes[x];
		while (*s!=0 && *t!=0 && *s==*t) { ++s; ++t; }
		if (*t==0) return true;
	}
	return false;
}

bool notValidNameCharacter(char c) {
	return !(isalnum(c) || c=='.' || c=='_' || c=='%' || c=='-' || c=='+');
}

bool notValidSiteCharacter(char c) {
	// NOTE: Kludge alert! The official spec for site characters is only
	// "a-zA-Z._%-". However, MDTest supports "international domain names,"
	// which use characters other than that; I'm kind of cheating here, handling
	// those by allowing all utf8-encoded characters too.
	return !(isalnum(c) || c=='.' || c=='_' || c=='%' || c=='-' || (c & 0x80));
}

bool isNotAlpha(char c) {
	return !isalpha(c);
}

std::string emailEncode(const std::string& src) {
	std::ostringstream out;
	bool inHex=false;
	for (std::string::const_iterator i=src.begin(), ie=src.end(); i!=ie;
		++i)
	{
		if (*i & 0x80) out << *i;
		else if (inHex) {
			out << "&#x" << std::hex << static_cast<int>(*i) << ';';
		} else {
			out << "&#" << std::dec << static_cast<int>(*i) << ';';
		}
		inHex=!inHex;
	}
	return out.str();
}

bool looksLikeEmailAddress(const std::string& str) {
	typedef std::string::const_iterator Iter;
	typedef std::string::const_reverse_iterator RIter;
	Iter i=std::find_if(str.begin(), str.end(), notValidNameCharacter);
	if (i!=str.end() && *i=='@' && i!=str.begin()) {
		// The name part is valid.
		i=std::find_if(i+1, str.end(), notValidSiteCharacter);
		if (i==str.end()) {
			// The site part doesn't contain any invalid characters.
			RIter ri=std::find_if(str.rbegin(), str.rend(), isNotAlpha);
			if (ri!=str.rend() && *ri=='.') {
				// It ends with a dot and only alphabetic characters.
				size_t d=std::distance(ri.base(), str.end());
				if (d>=2 && d<=4) {
					// There are two-to-four of them. It's valid.
					return true;
				}
			}
		}
	}
	return false;
}

// From <http://en.wikipedia.org/wiki/HTML_element>

const char *cOtherTagInit[]={
	// Header tags
	"title/", "base", "link", "basefont", "script/", "style/",
	"object/", "meta",

	// Inline tags
	"em/", "strong/", "q/", "cite/", "dfn/", "abbr/", "acronym/",
	"code/", "samp/", "kbd/", "var/", "sub/", "sup/", "del/", "ins/",
	"isindex", "a/", "img", "br", "map/", "area", "object/", "param",
	"applet/", "span/",

	0 };

const char *cBlockTagInit[]={ "p/", "blockquote/", "hr", "h1/", "h2/",
	"h3/", "h4/", "h5/", "h6/", "dl/", "dt/", "dd/", "ol/", "ul/",
	"li/", "dir/", "menu/", "table/", "tr/", "th/", "td/", "col",
	"colgroup/", "caption/", "thead/", "tbody/", "tfoot/", "form/",
	"select/", "option", "input", "label/", "textarea/", "div/", "pre/",
	"address/", "iframe/", "frame/", "frameset/", "noframes/",
	"center/", "b/", "i/", "big/", "small/", /*"s/",*/ "strike/", "tt/",
	"u/", "font/", "ins/", "del/", 0 };

// Other official ones (not presently in use in this code)
//"!doctype", "bdo", "body", "button", "fieldset", "head", "html",
//"legend", "noscript", "optgroup", "xmp",

boost::unordered_set<std::string> otherTags, blockTags;

void initTag(boost::unordered_set<std::string> &set, const char *init[]) {
	for (size_t x=0; init[x]!=0; ++x) {
		std::string str=init[x];
		if (*str.rbegin()=='/') {
			// Means it can have a closing tag
			str=str.substr(0, str.length()-1);
		}
		set.insert(str);
	}
}

std::string cleanTextLinkRef(const std::string& ref) {
	std::string r;
	for (std::string::const_iterator i=ref.begin(), ie=ref.end(); i!=ie;
		++i)
	{
		if (*i==' ') {
			if (r.empty() || *r.rbegin()!=' ') r.push_back(' ');
		} else r.push_back(*i);
	}
	return r;
}

} // namespace



size_t isValidTag(const std::string& tag, bool nonBlockFirst) {
	if (blockTags.empty()) {
		initTag(otherTags, cOtherTagInit);
		initTag(blockTags, cBlockTagInit);
	}

	if (nonBlockFirst) {
		if (otherTags.find(tag)!=otherTags.end()) return 1;
		if (blockTags.find(tag)!=blockTags.end()) return 2;
	} else {
		if (blockTags.find(tag)!=blockTags.end()) return 2;
		if (otherTags.find(tag)!=otherTags.end()) return 1;
	}
	return 0;
}



void TextHolder::writeAsHtml(std::ostream& out) const {
	preWrite(out);
	if (mEncodingFlags!=0) {
		out << encodeString(mText, mEncodingFlags);
	} else {
		out << mText;
	}
	postWrite(out);
}

optional<TokenGroup> RawText::processSpanElements(const LinkIds& idTable) {
	if (!canContainMarkup()) return none;

	ReplacementTable replacements;
	std::string str=_processHtmlTagAttributes(*text(), replacements);
	str=_processCodeSpans(str, replacements);
	str=_processEscapedCharacters(str);
	str=_processLinksImagesAndTags(str, replacements, idTable);
	return _processBoldAndItalicSpans(str, replacements);
}

std::string RawText::_processHtmlTagAttributes(std::string src, ReplacementTable&
	replacements)
{
	// Because "Attribute Content Is Not A Code Span"
	std::string tgt;
	std::string::const_iterator prev=src.begin(), end=src.end();
	while (1) {
		static const boost::regex cHtmlToken("<((/?)([a-zA-Z0-9]+)(?:( +[a-zA-Z0-9]+?(?: ?= ?(\"|').*?\\5))+? */? *))>");
		boost::smatch m;
		if (boost::regex_search(prev, end, m, cHtmlToken)) {
			// NOTE: Kludge alert! The `isValidTag` test is a cheat, only here
			// to handle some edge cases between the Markdown test suite and the
			// PHP-Markdown one, which seem to conflict.
			if (isValidTag(m[3])) {
				tgt+=std::string(prev, m[0].first);

				std::string fulltag=m[0], tgttag;
				std::string::const_iterator prevtag=fulltag.begin(), endtag=fulltag.end();
				while (1) {
					static const boost::regex cAttributeStrings("= ?(\"|').*?\\1");
					boost::smatch mtag;
					if (boost::regex_search(prevtag, endtag, mtag, cAttributeStrings)) {
						tgttag+=std::string(prevtag, mtag[0].first);
						tgttag+="\x01@"+boost::lexical_cast<std::string>(replacements.size())+"@htmlTagAttr\x01";
						prevtag=mtag[0].second;

						replacements.push_back(TokenPtr(new TextHolder(std::string(mtag[0]), false, cAmps|cAngles)));
					} else {
						tgttag+=std::string(prevtag, endtag);
						break;
					}
				}
				tgt+=tgttag;
				prev=m[0].second;
			} else {
				tgt+=std::string(prev, m[0].second);
				prev=m[0].second;
			}
		} else {
			tgt+=std::string(prev, end);
			break;
		}
	}

	return tgt;
}

std::string RawText::_processCodeSpans(std::string src, ReplacementTable&
	replacements)
{
	static const boost::regex cCodeSpan[2]={
		boost::regex("(?:^|(?<=[^\\\\]))`` (.+?) ``"),
		boost::regex("(?:^|(?<=[^\\\\]))`(.+?)`")
	};
	for (int pass=0; pass<2; ++pass) {
		std::string tgt;
		std::string::const_iterator prev=src.begin(), end=src.end();
		while (1) {
			boost::smatch m;
			if (boost::regex_search(prev, end, m, cCodeSpan[pass])) {
				tgt+=std::string(prev, m[0].first);
				tgt+="\x01@"+boost::lexical_cast<std::string>(replacements.size())+"@codeSpan\x01";
				prev=m[0].second;
				replacements.push_back(TokenPtr(new CodeSpan(_restoreProcessedItems(m[1], replacements))));
			} else {
				tgt+=std::string(prev, end);
				break;
			}
		}
		src.swap(tgt);
		tgt.clear();
	}
	return src;
}

std::string RawText::_processEscapedCharacters(const std::string& src) {
	std::string tgt;
	std::string::const_iterator prev=src.begin(), end=src.end();
	while (1) {
		std::string::const_iterator i=std::find(prev, end, '\\');
		if (i!=end) {
			tgt+=std::string(prev, i);
			++i;
			if (i!=end) {
				optional<size_t> e=isEscapedCharacter(*i);
				if (e) tgt+="\x01@#"+boost::lexical_cast<std::string>(*e)+"@escaped\x01";
				else tgt=tgt+'\\'+*i;
				prev=i+1;
			} else {
				tgt+='\\';
				break;
			}
		} else {
			tgt+=std::string(prev, end);
			break;
		}
	}
	return tgt;
}

std::string RawText::_processSpaceBracketedGroupings(const std::string &src,
	ReplacementTable& replacements)
{
	static const boost::regex cRemove("(?:(?: \\*+ )|(?: _+ ))");

	std::string tgt;
	std::string::const_iterator prev=src.begin(), end=src.end();
	while (1) {
		boost::smatch m;
		if (boost::regex_search(prev, end, m, cRemove)) {
			tgt+=std::string(prev, m[0].first);
			tgt+="\x01@"+boost::lexical_cast<std::string>(replacements.size())+"@spaceBracketed\x01";
			replacements.push_back(TokenPtr(new RawText(m[0])));
			prev=m[0].second;
		} else {
			tgt+=std::string(prev, end);
			break;
		}
	}
	return tgt;
}

std::string RawText::_processLinksImagesAndTags(const std::string &src,
	ReplacementTable& replacements, const LinkIds& idTable)
{
	// NOTE: Kludge alert! The "inline link or image" regex should be...
	//
	//   "(?:(!?)\\[(.+?)\\] *\\((.*?)\\))"
	//
	// ...but that fails on the 'Images' test because it includes a "stupid URL"
	// that has parentheses within it. The proper way to deal with this would be
	// to match any nested parentheses, but regular expressions can't handle an
	// unknown number of nested items, so I'm cheating -- the regex for it
	// allows for one (and *only* one) pair of matched parentheses within the
	// URL. It makes the regex hard to follow (it was even harder to get right),
	// but it allows it to pass the test.
	//
	// The "reference link or image" one has a similar problem; it should be...
	//
	//   "|(?:(!?)\\[(.+?)\\](?: *\\[(.*?)\\])?)"
	//
	static const boost::regex cExpression(
		"(?:(!?)\\[([^\\]]+?)\\] *\\(([^\\(]*(?:\\(.*?\\).*?)*?)\\))" // Inline link or image
		"|(?:(!?)\\[((?:[^]]*?\\[.*?\\].*?)|(?:.+?))\\](?: *\\[(.*?)\\])?)" // Reference link or image
		"|(?:<(/?([a-zA-Z0-9]+).*?)>)" // potential HTML tag or auto-link
	);
	// Important captures: 1/4=image indicator, 2/5=contents/alttext,
	// 3=URL/title, 6=optional link ID, 7=potential HTML tag or auto-link
	// contents, 8=actual tag from 7.

	std::string tgt;
	std::string::const_iterator prev=src.begin(), end=src.end();
	while (1) {
		boost::smatch m;
		if (boost::regex_search(prev, end, m, cExpression)) {
			assert(m[0].matched);
			assert(m[0].length()!=0);

			tgt+=std::string(prev, m[0].first);
			tgt+="\x01@"+boost::lexical_cast<std::string>(replacements.size())+"@links&Images1\x01";
			prev=m[0].second;

			bool isImage=false, isLink=false, isReference=false;
			if (m[4].matched && m[4].length()) isImage=isReference=true;
			else if (m[1].matched && m[1].length()) isImage=true;
			else if (m[5].matched) isLink=isReference=true;
			else if (m[2].matched) isLink=true;

			if (isImage || isLink) {
				std::string contentsOrAlttext, url, title;
				bool resolved=false;
				if (isReference) {
					contentsOrAlttext=m[5];
					std::string linkId=(m[6].matched ? std::string(m[6]) : std::string());
					if (linkId.empty()) linkId=cleanTextLinkRef(contentsOrAlttext);

					optional<markdown::LinkIds::Target> target=idTable.find(linkId);
					if (target) { url=target->url; title=target->title; resolved=true; };
				} else {
					static const boost::regex cReference("^<?([^ >]*)>?(?: *(?:('|\")(.*)\\2)|(?:\\((.*)\\)))? *$");
					// Useful captures: 1=url, 3/4=title
					contentsOrAlttext=m[2];
					std::string urlAndTitle=m[3];
					boost::smatch mm;
					if (boost::regex_match(urlAndTitle, mm, cReference)) {
						url=mm[1];
						if (mm[3].matched) title=mm[3];
						else if (mm[4].matched) title=mm[4];
						resolved=true;
					}
				}

				if (!resolved) {
					// Just encode the first character as-is, and continue
					// searching after it.
					prev=m[0].first+1;
					replacements.push_back(TokenPtr(new RawText(std::string(m[0].first, prev))));
				} else if (isImage) {
					replacements.push_back(TokenPtr(new Image(contentsOrAlttext,
						url, title)));
				} else {
					replacements.push_back(TokenPtr(new HtmlAnchorTag(url, title)));
					tgt+=contentsOrAlttext;
					tgt+="\x01@"+boost::lexical_cast<std::string>(replacements.size())+"@links&Images2\x01";
					replacements.push_back(TokenPtr(new HtmlTag("/a")));
				}
			} else {
				// Otherwise it's an HTML tag or auto-link.
				std::string contents=m[7];

//				cerr << "Evaluating potential HTML or auto-link: " << contents << endl;
//				cerr << "m[8]=" << m[8] << endl;

				if (looksLikeUrl(contents)) {
					TokenGroup subgroup;
					subgroup.push_back(TokenPtr(new HtmlAnchorTag(contents)));
					subgroup.push_back(TokenPtr(new RawText(contents, false)));
					subgroup.push_back(TokenPtr(new HtmlTag("/a")));
					replacements.push_back(TokenPtr(new Container(subgroup)));
				} else if (looksLikeEmailAddress(contents)) {
					TokenGroup subgroup;
					subgroup.push_back(TokenPtr(new HtmlAnchorTag(emailEncode("mailto:"+contents))));
					subgroup.push_back(TokenPtr(new RawText(emailEncode(contents), false)));
					subgroup.push_back(TokenPtr(new HtmlTag("/a")));
					replacements.push_back(TokenPtr(new Container(subgroup)));
				} else if (isValidTag(m[8])) {
					replacements.push_back(TokenPtr(new HtmlTag(_restoreProcessedItems(contents, replacements))));
				} else {
					// Just encode it as-is
					replacements.push_back(TokenPtr(new RawText(m[0])));
				}
			}
		} else {
			tgt+=std::string(prev, end);
			break;
		}
	}
	return tgt;
}

TokenGroup RawText::_processBoldAndItalicSpans(const std::string& src,
	ReplacementTable& replacements)
{
	static const boost::regex cEmphasisExpression(
		"(?:(?<![*_])([*_]{1,3})([^*_ ]+?)\\1(?![*_]))"                                    // Mid-word emphasis
		"|((?:(?<!\\*)\\*{1,3}(?!\\*)|(?<!_)_{1,3}(?!_))(?=.)(?! )(?![.,:;] )(?![.,:;]$))" // Open
		"|((?<![* ])\\*{1,3}(?!\\*)|(?<![ _])_{1,3}(?!_))"                                 // Close
	);

	TokenGroup tgt;
	std::string::const_iterator i=src.begin(), end=src.end(), prev=i;

	while (1) {
		boost::smatch m;
		if (boost::regex_search(prev, end, m, cEmphasisExpression)) {
			if (prev!=m[0].first) tgt.push_back(TokenPtr(new
				RawText(std::string(prev, m[0].first))));
			if (m[3].matched) {
				std::string token=m[3];
				tgt.push_back(TokenPtr(new BoldOrItalicMarker(true, token[0],
					token.length())));
				prev=m[0].second;
			} else if (m[4].matched) {
				std::string token=m[4];
				tgt.push_back(TokenPtr(new BoldOrItalicMarker(false, token[0],
					token.length())));
				prev=m[0].second;
			} else {
				std::string token=m[1], contents=m[2];
				tgt.push_back(TokenPtr(new BoldOrItalicMarker(true, token[0],
					token.length())));
				tgt.push_back(TokenPtr(new RawText(std::string(contents))));
				tgt.push_back(TokenPtr(new BoldOrItalicMarker(false, token[0],
					token.length())));
				prev=m[0].second;
			}
		} else {
			if (prev!=end) tgt.push_back(TokenPtr(new RawText(std::string(prev,
				end))));
			break;
		}
	}

	int id=0;
	for (TokenGroup::iterator ii=tgt.begin(), iie=tgt.end(); ii!=iie; ++ii) {
		if ((*ii)->isUnmatchedOpenMarker()) {
			BoldOrItalicMarker *openToken=dynamic_cast<BoldOrItalicMarker*>(ii->get());

			// Find a matching close-marker, if it's there
			TokenGroup::iterator iii=ii;
			for (++iii; iii!=iie; ++iii) {
				if ((*iii)->isUnmatchedCloseMarker()) {
					BoldOrItalicMarker *closeToken=dynamic_cast<BoldOrItalicMarker*>(iii->get());
					if (closeToken->size()==3 && openToken->size()!=3) {
						// Split the close-token into a match for the open-token
						// and a second for the leftovers.
						closeToken->disable();
						TokenGroup g;
						g.push_back(TokenPtr(new BoldOrItalicMarker(false,
							closeToken->tokenCharacter(), closeToken->size()-
							openToken->size())));
						g.push_back(TokenPtr(new BoldOrItalicMarker(false,
							closeToken->tokenCharacter(), openToken->size())));
						TokenGroup::iterator after=iii;
						++after;
						tgt.splice(after, g);
						continue;
					}

					if (closeToken->tokenCharacter()==openToken->tokenCharacter()
						&& closeToken->size()==openToken->size())
					{
						openToken->matched(closeToken, id);
						closeToken->matched(openToken, id);
						++id;
						break;
					} else if (openToken->size()==3) {
						// Split the open-token into a match for the close-token
						// and a second for the leftovers.
						openToken->disable();
						TokenGroup g;
						g.push_back(TokenPtr(new BoldOrItalicMarker(true,
							openToken->tokenCharacter(), openToken->size()-
							closeToken->size())));
						g.push_back(TokenPtr(new BoldOrItalicMarker(true,
							openToken->tokenCharacter(), closeToken->size())));
						TokenGroup::iterator after=ii;
						++after;
						tgt.splice(after, g);
						break;
					}
				}
			}
		}
	}

	// "Unmatch" invalidly-nested matches.
	std::stack<BoldOrItalicMarker*> openMatches;
	for (TokenGroup::iterator ii=tgt.begin(), iie=tgt.end(); ii!=iie; ++ii) {
		if ((*ii)->isMatchedOpenMarker()) {
			BoldOrItalicMarker *open=dynamic_cast<BoldOrItalicMarker*>(ii->get());
			openMatches.push(open);
		} else if ((*ii)->isMatchedCloseMarker()) {
			BoldOrItalicMarker *close=dynamic_cast<BoldOrItalicMarker*>(ii->get());

			if (close->id() != openMatches.top()->id()) {
				close->matchedTo()->matched(0);
				close->matched(0);
			} else {
				openMatches.pop();
				while (!openMatches.empty() && openMatches.top()->matchedTo()==0)
					openMatches.pop();
			}
		}
	}

	TokenGroup r;
	for (TokenGroup::iterator ii=tgt.begin(), iie=tgt.end(); ii!=iie; ++ii) {
		if ((*ii)->text() && (*ii)->canContainMarkup()) {
			TokenGroup t=_encodeProcessedItems(*(*ii)->text(), replacements);
			r.splice(r.end(), t);
		} else r.push_back(*ii);
	}

	return r;
}

TokenGroup RawText::_encodeProcessedItems(const std::string &src,
	ReplacementTable& replacements)
{
	static const boost::regex cReplaced("\x01@(#?[0-9]*)@.+?\x01");

	TokenGroup r;
	std::string::const_iterator prev=src.begin();
	while (1) {
		boost::smatch m;
		if (boost::regex_search(prev, src.end(), m, cReplaced)) {
			std::string pre=std::string(prev, m[0].first);
			if (!pre.empty()) r.push_back(TokenPtr(new RawText(pre)));
			prev=m[0].second;

			std::string ref=m[1];
			if (ref[0]=='#') {
				size_t n=boost::lexical_cast<size_t>(ref.substr(1));
				r.push_back(TokenPtr(new EscapedCharacter(escapedCharacter(n))));
			} else if (!ref.empty()) {
				size_t n=boost::lexical_cast<size_t>(ref);

				assert(n<replacements.size());
				r.push_back(replacements[n]);
			} // Otherwise just eat it
		} else {
			std::string pre=std::string(prev, src.end());
			if (!pre.empty()) r.push_back(TokenPtr(new RawText(pre)));
			break;
		}
	}
	return r;
}

std::string RawText::_restoreProcessedItems(const std::string &src,
	ReplacementTable& replacements)
{
	static const boost::regex cReplaced("\x01@(#?[0-9]*)@.+?\x01");

	std::ostringstream r;
	std::string::const_iterator prev=src.begin();
	while (1) {
		boost::smatch m;
		if (boost::regex_search(prev, src.end(), m, cReplaced)) {
			std::string pre=std::string(prev, m[0].first);
			if (!pre.empty()) r << pre;
			prev=m[0].second;

			std::string ref=m[1];
			if (ref[0]=='#') {
				size_t n=boost::lexical_cast<size_t>(ref.substr(1));
				r << '\\' << escapedCharacter(n);
			} else if (!ref.empty()) {
				size_t n=boost::lexical_cast<size_t>(ref);

				assert(n<replacements.size());
				replacements[n]->writeAsOriginal(r);
			} // Otherwise just eat it
		} else {
			std::string pre=std::string(prev, src.end());
			if (!pre.empty()) r << pre;
			break;
		}
	}
	return r.str();
}

HtmlAnchorTag::HtmlAnchorTag(const std::string& url, const std::string& title):
	TextHolder("<a href=\""+encodeString(url, cQuotes|cAmps)+"\""
		+(title.empty() ? std::string() : " title=\""+encodeString(title, cQuotes|cAmps)+"\"")
		+">", false, 0)
{
	// This space deliberately blank. ;-)
}

void CodeBlock::writeAsHtml(std::ostream& out) const {
	out << "<pre><code>";
	TextHolder::writeAsHtml(out);
	out << "</code></pre>\n\n";
}

void CodeSpan::writeAsHtml(std::ostream& out) const {
	out << "<code>";
	TextHolder::writeAsHtml(out);
	out << "</code>";
}

void CodeSpan::writeAsOriginal(std::ostream& out) const {
	out << '`' << *text() << '`';
}



void Container::writeAsHtml(std::ostream& out) const {
	preWrite(out);
	for (CTokenGroupIter i=mSubTokens.begin(), ie=mSubTokens.end(); i!=ie; ++i)
		(*i)->writeAsHtml(out);
	postWrite(out);
}

void Container::writeToken(size_t indent, std::ostream& out) const {
	out << std::string(indent*2, ' ') << containerName() << endl;
	for (CTokenGroupIter ii=mSubTokens.begin(), iie=mSubTokens.end(); ii!=iie;
		++ii)
			(*ii)->writeToken(indent+1, out);
}

optional<TokenGroup> Container::processSpanElements(const LinkIds& idTable) {
	TokenGroup t;
	for (CTokenGroupIter ii=mSubTokens.begin(), iie=mSubTokens.end(); ii!=iie;
		++ii)
	{
		if ((*ii)->text()) {
			optional<TokenGroup> subt=(*ii)->processSpanElements(idTable);
			if (subt) {
				if (subt->size()>1) t.push_back(TokenPtr(new Container(*subt)));
				else if (!subt->empty()) t.push_back(*subt->begin());
			} else t.push_back(*ii);
		} else {
			optional<TokenGroup> subt=(*ii)->processSpanElements(idTable);
			if (subt) {
				const Container *c=dynamic_cast<const Container*>((*ii).get());
				assert(c!=0);
				t.push_back(c->clone(*subt));
			} else t.push_back(*ii);
		}
	}
	swapSubtokens(t);
	return none;
}

UnorderedList::UnorderedList(const TokenGroup& contents, bool paragraphMode) {
	if (paragraphMode) {
		// Change each of the text items into paragraphs
		for (CTokenGroupIter i=contents.begin(), ie=contents.end(); i!=ie; ++i) {
			token::ListItem *item=dynamic_cast<token::ListItem*>((*i).get());
			assert(item!=0);
			item->inhibitParagraphs(false);
			mSubTokens.push_back(*i);
		}
	} else mSubTokens=contents;
}



void BoldOrItalicMarker::writeAsHtml(std::ostream& out) const {
	if (!mDisabled) {
		if (mMatch!=0) {
			assert(mSize>=1 && mSize<=3);
			if (mOpenMarker) {
				out << (mSize==1 ? "<em>" : mSize==2 ? "<strong>" : "<strong><em>");
			} else {
				out << (mSize==1 ? "</em>" : mSize==2 ? "</strong>" : "</em></strong>");
			}
		} else out << std::string(mSize, mTokenCharacter);
	}
}

void BoldOrItalicMarker::writeToken(std::ostream& out) const {
	if (!mDisabled) {
		if (mMatch!=0) {
			std::string type=(mSize==1 ? "italic" : mSize==2 ? "bold" : "italic&bold");
			if (mOpenMarker) {
				out << "Matched open-" << type << " marker" << endl;
			} else {
				out << "Matched close-" << type << " marker" << endl;
			}
		} else {
			if (mOpenMarker) out << "Unmatched bold/italic open marker: " <<
				std::string(mSize, mTokenCharacter) << endl;
			else out << "Unmatched bold/italic close marker: " <<
				std::string(mSize, mTokenCharacter) << endl;
		}
	}
}

void Image::writeAsHtml(std::ostream& out) const {
	out << "<img src=\"" << mUrl << "\" alt=\"" << mAltText << "\"";
	if (!mTitle.empty()) out << " title=\"" << mTitle << "\"";
	out << "/>";
}

} // namespace token
} // namespace markdown
