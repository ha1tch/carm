#!/usr/bin/env python3
# load_glove.py
#
# CARM -- Controlled Attention Routing and Masking
# Load GloVe embeddings and write binary format for CARM (Phase 1.2)
#
# Copyright (c) 2026 haitch
# Licensed under the Apache License, Version 2.0
# https://www.apache.org/licenses/LICENSE-2.0

"""
Load GloVe embeddings from gensim-data

This script loads pre-trained GloVe embeddings for use in the
Phase 1.2 embedding loader system.

Usage:
    python load_glove.py [--dimension 100|300]
"""

import argparse
import gensim.downloader as api
import numpy as np


def load_glove_embeddings(dimension=100):
    """
    Load GloVe embeddings from gensim-data
    
    Args:
        dimension: 100 or 300 (GloVe dimensionality)
    
    Returns:
        KeyedVectors model with loaded embeddings
    """
    model_name = f'glove-wiki-gigaword-{dimension}'
    
    print(f"Loading {model_name}...")
    print(f"This may take a few minutes on first run (downloads ~{330 if dimension==100 else 990} MB)")
    print(f"Subsequent runs will use cached data.")
    print()
    
    try:
        model = api.load(model_name)
        print(f"✓ Loaded {len(model)} words with {dimension}-dimensional embeddings")
        return model
    except Exception as e:
        print(f"✗ Error loading GloVe: {e}")
        return None


def get_embedding(model, word):
    """
    Get embedding for a word
    
    Args:
        model: Loaded KeyedVectors model
        word: Word to look up
    
    Returns:
        numpy array of embedding, or None if not found
    """
    try:
        return model[word.lower()]
    except KeyError:
        return None


def test_embeddings(model):
    """Test embeddings with sample medical/financial terms"""
    print("\n" + "="*60)
    print("Testing embeddings with sample terms:")
    print("="*60)
    
    test_words = [
        "aspirin", "ibuprofen", "headache", "fever",
        "billing", "invoice", "insurance", "payment",
        "patient", "doctor", "hospital", "clinic"
    ]
    
    found = 0
    for word in test_words:
        emb = get_embedding(model, word)
        if emb is not None:
            print(f"✓ {word:15s} - embedding shape: {emb.shape}")
            found += 1
        else:
            print(f"✗ {word:15s} - NOT FOUND")
    
    print(f"\nCoverage: {found}/{len(test_words)} ({100*found/len(test_words):.1f}%)")


def find_similar_words(model, word, topn=5):
    """Find most similar words to a given word"""
    try:
        similar = model.most_similar(word, topn=topn)
        print(f"\nMost similar to '{word}':")
        for sim_word, similarity in similar:
            print(f"  {sim_word:20s} (similarity: {similarity:.3f})")
    except KeyError:
        print(f"'{word}' not found in vocabulary")


def check_composition(model, word1, word2):
    """Test word composition (averaging)"""
    emb1 = get_embedding(model, word1)
    emb2 = get_embedding(model, word2)
    
    if emb1 is not None and emb2 is not None:
        composed = (emb1 + emb2) / 2
        
        # Find nearest words to composition
        print(f"\nComposition test: '{word1}' + '{word2}'")
        print(f"Nearest words to average embedding:")
        
        # Find most similar to composed embedding
        similar = model.similar_by_vector(composed, topn=5)
        for word, similarity in similar:
            print(f"  {word:20s} (similarity: {similarity:.3f})")
    else:
        print(f"Cannot compose: missing {word1 if emb1 is None else word2}")


def main():
    parser = argparse.ArgumentParser(description='Load GloVe embeddings')
    parser.add_argument('--dimension', type=int, choices=[100, 300], default=100,
                       help='GloVe dimensionality (100 or 300)')
    parser.add_argument('--test', action='store_true',
                       help='Run tests after loading')
    
    args = parser.parse_args()
    
    # Load embeddings
    model = load_glove_embeddings(args.dimension)
    
    if model is None:
        return 1
    
    # Run tests if requested
    if args.test:
        test_embeddings(model)
        
        print("\n" + "="*60)
        print("Similarity examples:")
        print("="*60)
        find_similar_words(model, "aspirin", topn=5)
        find_similar_words(model, "hospital", topn=5)
        
        print("\n" + "="*60)
        print("Composition examples:")
        print("="*60)
        check_composition(model, "billing", "code")
        check_composition(model, "patient", "record")
    
    print("\n" + "="*60)
    print("✓ GloVe embeddings loaded successfully")
    print("="*60)
    print(f"Vocabulary size: {len(model)}")
    print(f"Embedding dimension: {args.dimension}")
    print(f"Ready to use with build_vocabulary.py")
    
    return 0


if __name__ == "__main__":
    exit(main())
