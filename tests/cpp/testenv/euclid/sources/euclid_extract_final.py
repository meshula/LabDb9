#!/usr/bin/env python3
"""
euclid_extract_final.py  –  turn Heiberg's XML into structured output
Simple DOM-based approach for reliable text extraction

Usage:
    python euclid_extract_final.py  path/to/euclid.xml
"""

import argparse, unicodedata, yaml
from pathlib import Path
from lxml import etree
import betacode.conv as bc

def beta2uni(text: str) -> str:
    return bc.beta_to_uni(text)

def transliterate(beta: str) -> str:
    """Beta-code → NFC-normalised Unicode Greek (collapse whitespace)."""
    cleaned = " ".join(beta.split())
    return unicodedata.normalize("NFC", beta2uni(cleaned))

# ------------------------------------------------------------
def process(xml_path: Path, yml_path: Path, txt_path: Path):
    """Simple DOM-based processing"""
    print(f"Processing {xml_path} with DOM approach...")
    
    # Parse the entire document
    print("Parsing XML document...")
    tree = etree.parse(str(xml_path))
    root = tree.getroot()
    
    books = []
    bulk = []  # For transliteration scroll
    
    # Find all books
    book_elements = root.xpath("//div1[@type='book']")
    print(f"Found {len(book_elements)} books")
    
    for book_elem in book_elements:
        book_num = int(book_elem.get("n"))
        book_info = {
            "volume": book_num,
            "title": f"Book {book_num}",
            "xml-line": book_elem.sourceline,
            "propositions": []
        }
        
        # Find all propositions within this book
        # Look for div3[@type='number'] inside div2[@type='type' and @n='Prop']
        prop_elements = book_elem.xpath(".//div2[@type='type' and @n='Prop']//div3[@type='number']")
        
        print(f"  Book {book_num}: found {len(prop_elements)} propositions")
        
        for prop_elem in prop_elements:
            prop_no = int(prop_elem.get("n"))
            
            # Extract text from all <p> elements within this proposition
            paragraphs = []
            for p_elem in prop_elem.xpath(".//p"):
                # Get all text content, including from child elements
                p_text = "".join(p_elem.itertext()).strip()
                if p_text:  # Skip empty paragraphs
                    paragraphs.append(p_text)
            
            beta_txt = " ".join(paragraphs)
            uni_txt = transliterate(beta_txt)
            
            book_info["propositions"].append({
                "num": prop_no,
                "xml-line": prop_elem.sourceline,
                "text": uni_txt
            })
            
            # Debug first few propositions
            if len(books) == 0 and len(book_info["propositions"]) <= 3:
                print(f"    DEBUG: Prop {prop_no}: {len(paragraphs)} paragraphs, {len(uni_txt)} chars")
                if paragraphs:
                    print(f"    DEBUG: First text: '{paragraphs[0][:100]}...'")
                    print(f"    DEBUG: Transliterated: '{uni_txt[:100]}...'")
        
        # Sort propositions by number
        book_info["propositions"].sort(key=lambda p: p["num"])
        books.append(book_info)
    
    # Generate transliteration scroll from all <p> elements
    print("Generating transliteration scroll...")
    all_p_elements = root.xpath("//p")
    print(f"Found {len(all_p_elements)} <p> elements total")
    
    for p_elem in all_p_elements:
        beta_para = "".join(p_elem.itertext())
        if beta_para.strip():  # ignore empty
            bulk.append(f"# line {p_elem.sourceline}")
            bulk.append(transliterate(beta_para))
            bulk.append("")  # blank spacer line
    
    # Write YAML index
    yml_path.write_text(
        yaml.dump(books, allow_unicode=True, sort_keys=False),
        encoding="utf8"
    )
    
    # Write transliteration scroll
    txt_path.write_text("\\n".join(bulk), encoding="utf8")
    
    total_props = sum(len(book["propositions"]) for book in books)
    print(f"✔ Generated {yml_path} ({len(books)} books, {total_props} propositions)")
    print(f"✔ Generated {txt_path} ({len(bulk)} lines)")

# ============================================================
if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("xml", type=Path, help="Heiberg XML source")
    ap.add_argument("-y", "--yaml", default="euclid.yaml", type=Path)
    ap.add_argument("-t", "--txt",  default="euclid-translit.txt", type=Path)
    args = ap.parse_args()

    process(args.xml, args.yaml, args.txt)
    print("Processing complete!")
