#!/usr/bin/env python3
# build_vocabulary.py
#
# CARM -- Controlled Attention Routing and Masking
# Build concept vocabulary with TOSID assignments from a word list
#
# Copyright (c) 2026 haitch
# Licensed under the Apache License, Version 2.0
# https://www.apache.org/licenses/LICENSE-2.0

"""
Build Concept Database from Vocabulary + GloVe Embeddings

This script:
1. Reads vocabulary_1000.txt
2. Loads GloVe embeddings
3. Matches concepts to embeddings (with smart strategies)
4. Generates binary embedding file
5. Creates metadata JSON

Usage:
    python build_vocabulary.py [--dimension 100] [--output embeddings_glove_100]
"""

import argparse
import struct
import json
import numpy as np
import gensim.downloader as api
from datetime import datetime
from pathlib import Path


def load_glove(dimension=100):
    """Load GloVe embeddings"""
    model_name = f'glove-wiki-gigaword-{dimension}'
    print(f"Loading {model_name}...")
    model = api.load(model_name)
    print(f"✓ Loaded {len(model)} words\n")
    return model


def get_embedding_smart(model, concept_name):
    """
    Try multiple strategies to find embedding
    
    Returns: (embedding, method) or (None, "missing")
    
    Strategies:
    1. Exact match
    2. Lowercase
    3. Replace underscores with spaces
    4. Word composition (average)
    """
    
    # Strategy 1: Exact match
    if concept_name in model:
        return model[concept_name], "exact"
    
    # Strategy 2: Lowercase
    lower = concept_name.lower()
    if lower in model:
        return model[lower], "lowercase"
    
    # Strategy 3: Replace underscores with spaces
    if "_" in concept_name:
        space_version = concept_name.replace("_", " ")
        if space_version in model:
            return model[space_version], "space"
        
        # Try lowercase with spaces
        space_lower = space_version.lower()
        if space_lower in model:
            return model[space_lower], "space+lower"
        
        # Strategy 4: Word composition (for compound terms)
        words = concept_name.split("_")
        words_lower = [w.lower() for w in words]
        
        # Check if all words exist
        embeddings = []
        for w in words_lower:
            if w in model:
                embeddings.append(model[w])
            else:
                break  # Missing word, can't compose
        
        if len(embeddings) == len(words):
            # Average the embeddings
            composed = np.mean(embeddings, axis=0)
            return composed, f"composed({'+'.join(words_lower)})"
    
    # Strategy 5: Just try lowercase without underscore processing
    # (Handles cases like single-word concepts)
    if lower != concept_name and lower in model:
        return model[lower], "lowercase"
    
    # All strategies failed
    return None, "missing"


def parse_vocabulary(vocab_file):
    """
    Parse vocabulary_1000.txt
    
    Returns: list of dicts with concept info
    """
    concepts = []
    
    with open(vocab_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            
            # Parse: concept_name|tosid_full|domain|tosid_hex
            try:
                parts = line.split('|')
                if len(parts) != 4:
                    print(f"Warning: Line {line_num} has {len(parts)} fields (expected 4)")
                    continue
                
                concept_name, tosid_full, domain, tosid_hex = parts
                
                concepts.append({
                    'name': concept_name.strip(),
                    'tosid_full': tosid_full.strip(),
                    'domain': domain.strip(),
                    'tosid_hex': int(tosid_hex.strip(), 16)
                })
            except Exception as e:
                print(f"Error parsing line {line_num}: {e}")
                continue
    
    return concepts


def build_concept_database(vocab_file, glove_model, dimension):
    """
    Build concept database from vocabulary + embeddings
    
    Returns: (concepts_with_embeddings, missing_concepts, stats)
    """
    print(f"Reading vocabulary from {vocab_file}...")
    all_concepts = parse_vocabulary(vocab_file)
    print(f"✓ Read {len(all_concepts)} concepts\n")
    
    print("Matching concepts to embeddings...")
    print("="*70)
    
    concepts_with_embeddings = []
    missing_concepts = []
    method_counts = {}
    
    for i, concept in enumerate(all_concepts):
        name = concept['name']
        embedding, method = get_embedding_smart(glove_model, name)
        
        # Track methods
        method_counts[method] = method_counts.get(method, 0) + 1
        
        if embedding is not None:
            # Normalize embedding
            embedding = embedding / np.linalg.norm(embedding)
            
            concept['embedding'] = embedding
            concept['method'] = method
            concepts_with_embeddings.append(concept)
            
            # Print progress every 100 concepts
            if (i + 1) % 100 == 0:
                print(f"  Processed {i+1}/{len(all_concepts)} concepts...")
        else:
            missing_concepts.append(name)
            if len(missing_concepts) <= 10:  # Show first 10 missing
                print(f"  ✗ Missing: {name}")
    
    print("="*70)
    print()
    
    # Compute statistics
    stats = {
        'total_requested': len(all_concepts),
        'total_found': len(concepts_with_embeddings),
        'total_missing': len(missing_concepts),
        'coverage_percent': 100.0 * len(concepts_with_embeddings) / len(all_concepts),
        'method_breakdown': method_counts,
        'dimension': dimension
    }
    
    return concepts_with_embeddings, missing_concepts, stats


def save_binary(concepts, output_file, dimension):
    """
    Save concepts in binary format
    
    Binary format:
    - Header: [version:4][count:4][dim:4][reserved:4]
    - Concepts: [tosid:4][name:64][embedding:dim*4]...
    """
    print(f"Writing binary file: {output_file}...")
    
    with open(output_file, 'wb') as f:
        # Header
        version = 1
        count = len(concepts)
        
        f.write(struct.pack('I', version))    # Version
        f.write(struct.pack('I', count))      # Count
        f.write(struct.pack('I', dimension))  # Dimension
        f.write(struct.pack('I', 0))          # Reserved
        
        # Concepts
        for concept in concepts:
            # TOSID (4 bytes)
            f.write(struct.pack('I', concept['tosid_hex']))
            
            # Name (64 bytes, null-padded)
            name_bytes = concept['name'].encode('utf-8')[:63]
            name_padded = name_bytes.ljust(64, b'\0')
            f.write(name_padded)
            
            # Embedding (dim * 4 bytes)
            embedding = concept['embedding'].astype(np.float32)
            f.write(embedding.tobytes())
    
    file_size_mb = Path(output_file).stat().st_size / (1024 * 1024)
    print(f"✓ Wrote {count} concepts ({file_size_mb:.2f} MB)\n")


def save_text(concepts, output_file):
    """
    Save concepts in text format (for debugging/inspection)
    
    Format: name | tosid_hex | tosid_full | domain | method
    """
    print(f"Writing text file: {output_file}...")
    
    with open(output_file, 'w') as f:
        f.write("# Concept Database (Text Format)\n")
        f.write("# name | tosid_hex | tosid_full | domain | embedding_method\n")
        f.write("#\n")
        
        for concept in concepts:
            f.write(f"{concept['name']}|")
            f.write(f"0x{concept['tosid_hex']:08X}|")
            f.write(f"{concept['tosid_full']}|")
            f.write(f"{concept['domain']}|")
            f.write(f"{concept['method']}\n")
    
    print(f"✓ Wrote {len(concepts)} concepts\n")


def save_metadata(stats, missing_concepts, output_file, source):
    """Save metadata JSON with statistics"""
    print(f"Writing metadata: {output_file}...")
    
    metadata = {
        'build_date': datetime.now().isoformat(),
        'source': source,
        'statistics': stats,
        'missing_concepts': missing_concepts[:50],  # First 50
        'missing_count': len(missing_concepts)
    }
    
    with open(output_file, 'w') as f:
        json.dump(metadata, f, indent=2)
    
    print(f"✓ Wrote metadata\n")


def print_summary(stats, missing_concepts):
    """Print build summary"""
    print("="*70)
    print("BUILD SUMMARY")
    print("="*70)
    print(f"Total requested:      {stats['total_requested']}")
    print(f"Successfully matched: {stats['total_found']} ({stats['coverage_percent']:.1f}%)")
    print(f"Missing:              {stats['total_missing']} ({100-stats['coverage_percent']:.1f}%)")
    print()
    
    print("Matching methods used:")
    for method, count in sorted(stats['method_breakdown'].items(), key=lambda x: -x[1]):
        if method != "missing":
            percent = 100.0 * count / stats['total_found']
            print(f"  {method:20s}: {count:4d} ({percent:5.1f}%)")
    print()
    
    if missing_concepts:
        print(f"First 20 missing concepts:")
        for i, name in enumerate(missing_concepts[:20]):
            print(f"  {i+1:2d}. {name}")
        if len(missing_concepts) > 20:
            print(f"  ... and {len(missing_concepts)-20} more")
        print()
    
    # Quality assessment
    coverage = stats['coverage_percent']
    if coverage >= 95:
        status = "✓ EXCELLENT"
        color = "\033[32m"  # Green
    elif coverage >= 90:
        status = "✓ GOOD"
        color = "\033[33m"  # Yellow
    elif coverage >= 85:
        status = "⚠ ACCEPTABLE"
        color = "\033[33m"  # Yellow
    else:
        status = "✗ NEEDS REVIEW"
        color = "\033[31m"  # Red
    reset = "\033[0m"
    
    print(f"Coverage Quality: {color}{status}{reset} ({coverage:.1f}%)")
    
    if coverage < 95:
        print()
        print("Recommendations:")
        print("  - Review missing concepts list")
        print("  - Consider alternative vocabulary choices")
        print("  - May need to reselect some concepts")
    
    print("="*70)


def main():
    parser = argparse.ArgumentParser(description='Build concept database from vocabulary + GloVe')
    parser.add_argument('--dimension', type=int, choices=[100, 300], default=100,
                       help='GloVe dimensionality (100 or 300)')
    parser.add_argument('--vocab', type=str, default='../embeddings/vocabulary_1000.txt',
                       help='Path to vocabulary file')
    parser.add_argument('--output', type=str, default=None,
                       help='Output file prefix (default: embeddings_glove_{dim})')
    
    args = parser.parse_args()
    
    # Determine output prefix
    if args.output is None:
        output_prefix = f"../embeddings/embeddings_glove_{args.dimension}"
    else:
        output_prefix = args.output
    
    # Load GloVe
    glove_model = load_glove(args.dimension)
    
    # Build database
    concepts, missing, stats = build_concept_database(
        args.vocab, glove_model, args.dimension)
    
    # Save files
    save_binary(concepts, f"{output_prefix}.bin", args.dimension)
    save_text(concepts, f"{output_prefix}.txt")
    save_metadata(stats, missing, f"{output_prefix}_metadata.json",
                 f"glove-wiki-gigaword-{args.dimension}")
    
    # Print summary
    print_summary(stats, missing)
    
    # Return success if coverage > 85%
    if stats['coverage_percent'] >= 85:
        print("\n✓ Build successful! Ready to use with C loader.")
        return 0
    else:
        print("\n⚠ Build completed with low coverage. Review missing concepts.")
        return 1


if __name__ == "__main__":
    exit(main())
