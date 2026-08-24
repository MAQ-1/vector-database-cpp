import React, { useState, useEffect, useRef } from 'react';
import './App.css';

// API Configuration
const API_BASE_URL = import.meta.env.VITE_API_URL || '/api';
const API = {
  status: `${API_BASE_URL}/status`,
  upload: `${API_BASE_URL}/doc/upload`,
  ask: `${API_BASE_URL}/doc/ask/compare`,
};

// Error Boundary for answer section
class AnswerErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, errorInfo) {
    console.error('Answer rendering error:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="mt-4 border border-[var(--error)] bg-[var(--error)]/10 p-4">
          <div className="flex items-center gap-2">
            <span className="text-[var(--error)] font-mono text-sm">✗</span>
            <span className="text-sm text-[var(--text-primary)]">
              Error rendering answer. Check console for details.
            </span>
          </div>
        </div>
      );
    }
    return this.props.children;
  }
}

function App() {
  // Theme state
  const [theme, setTheme] = useState(() => {
    const saved = localStorage.getItem('theme');
    return saved || 'dark';
  });

  // Backend status
  const [backendOnline, setBackendOnline] = useState(false);
  const [ollamaOnline, setOllamaOnline] = useState(false);

  // Document state
  const [selectedFile, setSelectedFile] = useState(null);
  const [documentInfo, setDocumentInfo] = useState(null);
  const [uploadError, setUploadError] = useState(null);
  const [uploadStatus, setUploadStatus] = useState('idle');

  // Indexing pipeline state
  const [pipelineSteps, setPipelineSteps] = useState([
    { id: 'upload', label: 'PDF Uploaded', state: 'pending' },
    { id: 'extract', label: 'Extracting Text', state: 'pending' },
    { id: 'chunk', label: 'Creating Chunks', state: 'pending' },
    { id: 'embed', label: 'Generating Embeddings', state: 'pending' },
    { id: 'index', label: 'Building HNSW Index', state: 'pending' },
    { id: 'ready', label: 'Ready for Questions', state: 'pending' },
  ]);

  // Question state
  const [question, setQuestion] = useState('');
  const [isAsking, setIsAsking] = useState(false);
  const [answer, setAnswer] = useState(null);
  const [answerMetadata, setAnswerMetadata] = useState(null);
  const [answerError, setAnswerError] = useState(null);

  // Search engine results
  const [engineResults, setEngineResults] = useState({
    hnsw: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
    kdTree: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
    bruteForce: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
  });

  const fileInputRef = useRef(null);
  const [isDragging, setIsDragging] = useState(false);

  // Apply theme to document
  useEffect(() => {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
  }, [theme]);

  // Toggle theme
  const toggleTheme = () => {
    setTheme(theme === 'dark' ? 'light' : 'dark');
  };

  // Check backend status on mount
  useEffect(() => {
    checkBackendStatus();
  }, []);

  const checkBackendStatus = async () => {
    try {
      const response = await fetch(API.status, { method: 'GET' });
      const data = await response.json();
      setBackendOnline(response.ok);
      if (data && data.ollama) {
        setOllamaOnline(data.ollama === 'ONLINE');
      }
    } catch (error) {
      setBackendOnline(false);
      setOllamaOnline(false);
      console.error('[FRONTEND] Backend check failed:', error);
    }
  };

  // Update pipeline step state
  const updatePipelineStep = (stepId, state, error = null) => {
    setPipelineSteps(prev => prev.map(step => 
      step.id === stepId ? { ...step, state, error } : step
    ));
  };

  // Reset pipeline
  const resetPipeline = () => {
    setPipelineSteps([
      { id: 'upload', label: 'PDF Uploaded', state: 'pending' },
      { id: 'extract', label: 'Extracting Text', state: 'pending' },
      { id: 'chunk', label: 'Creating Chunks', state: 'pending' },
      { id: 'embed', label: 'Generating Embeddings', state: 'pending' },
      { id: 'index', label: 'Building HNSW Index', state: 'pending' },
      { id: 'ready', label: 'Ready for Questions', state: 'pending' },
    ]);
  };

  // File upload handlers
  const handleFileSelect = (event) => {
    const file = event.target.files?.[0];
    if (file) {
      validateAndUploadFile(file);
    }
  };

  const handleDrop = (event) => {
    event.preventDefault();
    setIsDragging(false);
    const file = event.dataTransfer.files?.[0];
    if (file) {
      validateAndUploadFile(file);
    }
  };

  const handleDragOver = (event) => {
    event.preventDefault();
    setIsDragging(true);
  };

  const handleDragLeave = () => {
    setIsDragging(false);
  };

  const validateAndUploadFile = (file) => {
    if (file.type !== 'application/pdf') {
      setUploadError('Only PDF files are accepted');
      setTimeout(() => setUploadError(null), 3000);
      return;
    }

    setSelectedFile(file);
    setUploadError(null);
    uploadFile(file);
  };

  const uploadFile = async (file) => {
    resetPipeline();
    setDocumentInfo(null);
    setUploadError(null);
    setUploadStatus('uploading');
    setAnswer(null);
    setAnswerMetadata(null);
    setAnswerError(null);
    setEngineResults({
      hnsw: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
      kdTree: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
      bruteForce: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
    });

    try {
      const formData = new FormData();
      formData.append('file', file);

      updatePipelineStep('upload', 'active');

      const response = await fetch(API.upload, {
        method: 'POST',
        body: formData,
      });

      if (!response.ok) {
        const errorData = await response.json().catch(() => ({ error: 'Upload failed' }));
        throw new Error(errorData.error || `Upload failed with status ${response.status}`);
      }

      const data = await response.json();

      if (!data.success) {
        throw new Error(data.error || 'Upload failed');
      }

      updatePipelineStep('upload', 'completed');
      setUploadStatus('indexing');

      // Simulate pipeline progress
      const steps = [
        { id: 'extract', label: 'Extracting Text' },
        { id: 'chunk', label: 'Creating Chunks' },
        { id: 'embed', label: 'Generating Embeddings' },
        { id: 'index', label: 'Building HNSW Index' },
      ];

      for (const step of steps) {
        updatePipelineStep(step.id, 'active');
        await new Promise(resolve => setTimeout(resolve, 600));
        updatePipelineStep(step.id, 'completed');
      }

      updatePipelineStep('ready', 'active');
      await new Promise(resolve => setTimeout(resolve, 300));
      updatePipelineStep('ready', 'completed');

      setDocumentInfo({
        id: data.id,
        filename: file.name,
        size: formatFileSize(file.size),
        characters: data.characters_extracted || null,
      });
      setUploadStatus('indexed');
    } catch (error) {
      console.error('[FRONTEND] Upload error:', error);
      
      setPipelineSteps(prev => prev.map(step => 
        step.state === 'active' ? { ...step, state: 'error', error: error.message } : step
      ));
      
      setUploadError(error.message || 'Failed to upload document');
      setUploadStatus('error');
    }
  };

  const handleRemoveDocument = async () => {
    if (documentInfo && documentInfo.id) {
      try {
        await fetch(`${API.upload}/${documentInfo.id}`, { method: 'DELETE' }).catch(() => {});
      } catch (error) {
        console.error('Failed to remove document from backend:', error);
      }
    }

    setSelectedFile(null);
    setDocumentInfo(null);
    setUploadError(null);
    setUploadStatus('idle');
    resetPipeline();
    setQuestion('');
    setAnswer(null);
    setAnswerMetadata(null);
    setAnswerError(null);
    setEngineResults({
      hnsw: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
      kdTree: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
      bruteForce: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
    });

    if (fileInputRef.current) {
      fileInputRef.current.value = '';
    }
  };

  const handleAskQuestion = async () => {
    const isReady = pipelineSteps.every(step => step.state === 'completed');
    if (!question.trim() || !isReady || isAsking) return;

    setIsAsking(true);
    setAnswer(null);
    setAnswerMetadata(null);
    setAnswerError(null);

    // Set all engines to SEARCHING
    setEngineResults({
      hnsw: { status: 'SEARCHING', time: null, chunks: null, error: null, fastest: false },
      kdTree: { status: 'SEARCHING', time: null, chunks: null, error: null, fastest: false },
      bruteForce: { status: 'SEARCHING', time: null, chunks: null, error: null, fastest: false },
    });

    try {
      const response = await fetch(API.ask, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          question: question.trim(),
        }),
      });

      if (!response.ok) {
        const errorData = await response.json().catch(() => ({ error: 'Failed to get answer' }));
        throw new Error(errorData.error || 'Failed to get answer');
      }

      const data = await response.json();

      if (!data || typeof data !== 'object') {
        throw new Error('Invalid response format from server');
      }

      if (data.success === false) {
        throw new Error(data.error || 'Request failed');
      }

      let updatedResults = {
        hnsw: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
        kdTree: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
        bruteForce: { status: 'READY', time: null, chunks: null, error: null, fastest: false },
      };

      let fastestEngine = null;
      let fastestTime = Infinity;
      let bestAnswer = null;
      let bestSources = [];

      const algorithms = Array.isArray(data.algorithms) ? data.algorithms : [];

      if (algorithms.length === 0) {
        if (data.answer && typeof data.answer === 'string') {
          setAnswer(data.answer);
          setAnswerMetadata({
            engine: 'UNKNOWN',
            time: null,
            sources: [],
          });
        } else {
          setAnswer('No relevant results found in this document');
        }
      } else {
        algorithms.forEach(algo => {
          const algoName = algo.algorithm?.toUpperCase() || '';
          let key = null;

          if (algoName === 'HNSW') key = 'hnsw';
          else if (algoName === 'KD-TREE' || algoName === 'KDTREE') key = 'kdTree';
          else if (algoName === 'BRUTE_FORCE' || algoName === 'BRUTE FORCE') key = 'bruteForce';

          if (key) {
            const timeValue = algo.search_time_ms !== undefined && algo.search_time_ms !== null 
              ? parseFloat(algo.search_time_ms) 
              : null;
            
            updatedResults[key] = {
              status: algo.success ? 'COMPLETE' : 'ERROR',
              time: timeValue !== null && !isNaN(timeValue) ? timeValue : null,
              chunks: algo.chunks_retrieved !== undefined && algo.chunks_retrieved !== null ? algo.chunks_retrieved : null,
              error: algo.error || null,
              fastest: false,
            };

            if (algo.success && timeValue !== null && !isNaN(timeValue) && timeValue < fastestTime) {
              fastestTime = timeValue;
              fastestEngine = key;
              bestAnswer = algo.answer || null;
              bestSources = Array.isArray(algo.sources) ? algo.sources : [];
            }
          }
        });

        if (fastestEngine) {
          updatedResults[fastestEngine].fastest = true;
          
          if (typeof bestAnswer === 'string' && bestAnswer.trim()) {
            setAnswer(bestAnswer);
          } else {
            setAnswer('No answer generated');
          }
          
          setAnswerMetadata({
            engine: fastestEngine,
            time: fastestTime,
            sources: bestSources,
          });
        } else {
          const hasError = algorithms.some(algo => !algo.success);
          if (hasError) {
            const errors = algorithms
              .filter(algo => !algo.success)
              .map(algo => algo.error)
              .filter(Boolean);
            setAnswer(`Search failed: ${errors.join(', ') || 'All search engines failed'}`);
          } else {
            setAnswer('No relevant results found in this document');
          }
          setAnswerMetadata(null);
        }
      }

      setEngineResults(updatedResults);
    } catch (error) {
      console.error('[FRONTEND] Ask error:', error);
      setAnswerError(error.message || 'Failed to get answer');
      
      setEngineResults({
        hnsw: { status: 'ERROR', time: null, chunks: null, error: error.message, fastest: false },
        kdTree: { status: 'ERROR', time: null, chunks: null, error: error.message, fastest: false },
        bruteForce: { status: 'ERROR', time: null, chunks: null, error: error.message, fastest: false },
      });
    } finally {
      setIsAsking(false);
    }
  };

  const formatFileSize = (bytes) => {
    if (!bytes || bytes < 0) return '0 B';
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  };

  const isDocumentReady = () => {
    return pipelineSteps.every(step => step.state === 'completed');
  };

  const getStatusColor = (status) => {
    switch (status) {
      case 'READY': return 'text-[var(--text-secondary)]';
      case 'SEARCHING': return 'text-[var(--processing)]';
      case 'COMPLETE': return 'text-[var(--success)]';
      case 'ERROR': return 'text-[var(--error)]';
      default: return 'text-[var(--text-secondary)]';
    }
  };

  const getEngineName = (key) => {
    const names = {
      hnsw: 'HNSW',
      kdTree: 'KD-Tree',
      bruteForce: 'Brute Force',
    };
    return names[key] || key;
  };

  const getEngineDescription = (key) => {
    const descriptions = {
      hnsw: 'Approximate nearest neighbor search',
      kdTree: 'Space-partitioning vector search',
      bruteForce: 'Exact vector comparison',
    };
    return descriptions[key] || '';
  };

  const getEngineColor = (key) => {
    const colors = {
      hnsw: 'var(--hnsw)',
      kdTree: 'var(--kd-tree)',
      bruteForce: 'var(--brute-force)',
    };
    return colors[key] || 'var(--text-secondary)';
  };

  // Pixel Icons
  const Icon = ({ type, className }) => {
    const icons = {
      database: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <rect x="3" y="4" width="18" height="16" rx="0" />
          <path d="M3 8h18" />
          <path d="M3 12h18" />
          <path d="M3 16h18" />
        </svg>
      ),
      upload: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <path d="M12 16v-8" />
          <path d="M8 12l4-4 4 4" />
          <rect x="3" y="14" width="18" height="6" rx="0" />
        </svg>
      ),
      document: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <path d="M6 4h8l6 6v10a2 2 0 01-2 2H6a2 2 0 01-2-2V6a2 2 0 012-2z" />
          <path d="M14 4v6h6" />
        </svg>
      ),
      chat: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <path d="M21 11.5a8.38 8.38 0 01-.9 3.8 8.5 8.5 0 01-7.6 4.7 8.38 8.38 0 01-3.8-.9L3 21l1.9-5.7a8.38 8.38 0 01-.9-3.8 8.5 8.5 0 014.7-7.6 8.38 8.38 0 013.8-.9h.5a8.48 8.48 0 018 8v.5z" />
        </svg>
      ),
      hnsw: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <circle cx="12" cy="6" r="2" />
          <circle cx="6" cy="18" r="2" />
          <circle cx="18" cy="18" r="2" />
          <path d="M12 8v4" />
          <path d="M8 18l-2-2" />
          <path d="M16 18l2-2" />
          <path d="M6 16l4-4" />
          <path d="M18 16l-4-4" />
        </svg>
      ),
      kdtree: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <path d="M3 3l18 18" />
          <path d="M3 21l18-18" />
          <path d="M12 3v18" />
          <path d="M3 12h18" />
        </svg>
      ),
      brute: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <rect x="2" y="2" width="20" height="20" rx="0" />
          <path d="M6 2v20" />
          <path d="M18 2v20" />
          <path d="M2 6h20" />
          <path d="M2 18h20" />
        </svg>
      ),
      sun: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <circle cx="12" cy="12" r="5" />
          <path d="M12 1v2" />
          <path d="M12 21v2" />
          <path d="M4.22 4.22l1.42 1.42" />
          <path d="M18.36 18.36l1.42 1.42" />
          <path d="M1 12h2" />
          <path d="M21 12h2" />
          <path d="M4.22 19.78l1.42-1.42" />
          <path d="M18.36 5.64l1.42-1.42" />
        </svg>
      ),
      moon: (
        <svg className={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="square">
          <path d="M21 12.79A9 9 0 1111.21 3 7 7 0 0021 12.79z" />
        </svg>
      ),
    };
    return icons[type] || null;
  };

  return (
    <div className="min-h-screen bg-[var(--bg-primary)] text-[var(--text-primary)] transition-colors duration-200">
      {/* Subtle grid background */}
      <div className="fixed inset-0 pointer-events-none opacity-[var(--grid-opacity)]">
        <div className="w-full h-full" style={{
          backgroundImage: `radial-gradient(circle at 1px 1px, var(--grid-color) 1px, transparent 0)`,
          backgroundSize: '16px 16px'
        }}></div>
      </div>

      <div className="relative max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-6">
        {/* Header */}
        <header className="flex flex-wrap items-center justify-between gap-4 mb-6 pb-4 border-b border-[var(--border)]">
          <div className="flex items-center gap-3">
            <div className="w-9 h-9 border border-[var(--border)] flex items-center justify-center">
              <Icon type="database" className="w-5 h-5 text-[var(--kd-tree)]" />
            </div>
            <div>
              <h1 className="text-lg font-bold font-mono tracking-tight">VectorDB</h1>
              <p className="text-[10px] text-[var(--text-secondary)] font-mono tracking-wider">BUILT FROM SCRATCH IN C++</p>
            </div>
          </div>

          <div className="flex items-center gap-4 flex-wrap">
            <div className="flex items-center gap-4 text-sm font-mono">
              <div className="flex items-center gap-2">
                <span className={`w-3 h-3 border border-current ${backendOnline ? 'bg-[var(--success)] border-[var(--success)]' : 'bg-[var(--error)] border-[var(--error)]'}`}></span>
                <span className="text-[var(--text-secondary)] text-xs">Backend: {backendOnline ? 'Online' : 'Offline'}</span>
              </div>
              <div className="flex items-center gap-2">
                <span className={`w-3 h-3 border border-current ${ollamaOnline ? 'bg-[var(--success)] border-[var(--success)]' : 'bg-[var(--error)] border-[var(--error)]'}`}></span>
                <span className="text-[var(--text-secondary)] text-xs">Ollama: {ollamaOnline ? 'Online' : 'Offline'}</span>
              </div>
            </div>

            {/* Theme Toggle Button */}
            <button
              onClick={toggleTheme}
              className="w-9 h-9 flex items-center justify-center border border-[var(--border)] bg-[var(--panel)] text-[var(--text-primary)] hover:border-[var(--kd-tree)] active:translate-x-[2px] active:translate-y-[2px] active:shadow-none transition-all shadow-[4px_4px_0_rgba(0,0,0,0.15)]"
              aria-label="Toggle theme"
            >
              {theme === 'dark' ? (
                <Icon type="sun" className="w-4 h-4 text-[var(--processing)]" />
              ) : (
                <Icon type="moon" className="w-4 h-4 text-[var(--processing)]" />
              )}
            </button>
          </div>
        </header>

        {/* Error Banner */}
        {uploadError && (
          <div className="mb-6 border border-[var(--error)] bg-[var(--error)]/10 px-4 py-2">
            <p className="text-sm text-[var(--error)] font-mono">{uploadError}</p>
          </div>
        )}

        {/* Main Content */}
        <main>
          {/* ROW 1: Document Pipeline | Ask Your Document - USING top-grid CLASS */}
          <div className="top-grid">
            {/* LEFT: Document Pipeline */}
            <div className="main-panel">
              <div className="panel-title">
                <Icon type="document" className="w-4 h-4 text-[var(--kd-tree)]" />
                <span>Document Pipeline</span>
              </div>

              {!selectedFile ? (
                <div
                  className={`upload-zone ${isDragging ? 'drag-active' : ''}`}
                  onClick={() => fileInputRef.current?.click()}
                  onDrop={handleDrop}
                  onDragOver={handleDragOver}
                  onDragLeave={handleDragLeave}
                >
                  <Icon type="upload" className="w-12 h-12 text-[var(--text-secondary)]" />
                  <p className="upload-title">Drop a PDF here or click to browse</p>
                  <p className="upload-subtitle font-mono">Only PDF files are accepted</p>
                  <input
                    ref={fileInputRef}
                    type="file"
                    accept="application/pdf"
                    onChange={handleFileSelect}
                    className="hidden"
                  />
                </div>
              ) : (
                <div>
                  {/* File Info Row */}
                  <div className="flex items-center justify-between p-3 border border-[var(--border)] bg-[var(--raised)]">
                    <div className="flex items-center gap-3 min-w-0">
                      <Icon type="document" className="w-5 h-5 text-[var(--error)] flex-shrink-0" />
                      <div className="min-w-0">
                        <p className="text-sm font-medium truncate">{selectedFile.name}</p>
                        <p className="text-xs text-[var(--text-secondary)] font-mono">{formatFileSize(selectedFile.size)}</p>
                      </div>
                    </div>
                    <button
                      onClick={handleRemoveDocument}
                      className="w-7 h-7 flex items-center justify-center border border-[var(--error)] text-[var(--error)] hover:bg-[var(--error)] hover:text-[var(--bg-primary)] transition-colors font-mono text-sm"
                      aria-label="Remove document"
                    >
                      ✕
                    </button>
                  </div>

                  {/* Pipeline Timeline */}
                  <div className="mt-4 pipeline">
                    {pipelineSteps.map((step, index) => {
                      const isActive = step.state === 'active';
                      const isCompleted = step.state === 'completed';
                      const isError = step.state === 'error';
                      
                      const stepClass = isActive ? 'processing' : isCompleted ? 'completed' : isError ? 'error' : 'pending';
                      
                      return (
                        <div key={step.id} className={`pipeline-step ${stepClass}`}>
                          <div className="pipeline-step-icon">
                            {isCompleted && '✓'}
                            {isActive && ''}
                            {isError && '!'}
                            {!isCompleted && !isActive && !isError && ''}
                          </div>
                          <span className="font-mono">
                            {step.label}
                            {isActive && (
                              <span className="ml-2 text-xs text-[var(--processing)]">
                                {step.id === 'upload' && 'Uploading...'}
                                {step.id === 'extract' && 'Extracting...'}
                                {step.id === 'chunk' && 'Creating chunks...'}
                                {step.id === 'embed' && 'Generating embeddings...'}
                                {step.id === 'index' && 'Building index...'}
                                {step.id === 'ready' && 'Preparing...'}
                              </span>
                            )}
                            {isError && step.error && (
                              <span className="ml-2 text-xs text-[var(--error)]">
                                ({step.error})
                              </span>
                            )}
                            {isCompleted && step.id === 'ready' && (
                              <span className="ml-2 text-xs text-[var(--success)]">Ready</span>
                            )}
                          </span>
                        </div>
                      );
                    })}
                  </div>
                </div>
              )}
            </div>

            {/* RIGHT: Ask Your Document */}
            <div className="ask-panel main-panel">
              <div className="panel-title">
                <Icon type="chat" className="w-4 h-4 text-[var(--kd-tree)]" />
                <span>Ask Your Document</span>
              </div>

              <textarea
                value={question}
                onChange={(e) => setQuestion(e.target.value)}
                placeholder="What would you like to know about this document?"
                disabled={!isDocumentReady() || isAsking}
                className="question-input font-mono"
              />

              <button
                onClick={handleAskQuestion}
                type="button"
                disabled={!isDocumentReady() || !question.trim() || isAsking}
                className="ask-button font-mono"
              >
                {isAsking ? (
                  <>
                    <div className="w-4 h-4 rounded-full border-2 border-[var(--bg-primary)] border-t-transparent animate-spin"></div>
                    Searching...
                  </>
                ) : (
                  <>
                    <span className="text-sm">▶</span>
                    Ask AI
                  </>
                )}
              </button>

              {/* AI Answer Panel */}
              <AnswerErrorBoundary>
                {answerError && (
                  <div className="mt-4 border border-[var(--error)] bg-[var(--error)]/10 p-4">
                    <p className="text-sm text-[var(--error)] font-mono">{answerError}</p>
                  </div>
                )}

                {answer && (
                  <div className="answer-card">
                    <div className="answer-header">
                      <div className="flex items-center gap-2">
                        <Icon type="chat" className="w-4 h-4 text-[var(--kd-tree)]" />
                        <span className="text-sm font-bold font-mono">AI Answer</span>
                      </div>
                      {answerMetadata && answerMetadata.engine !== 'UNKNOWN' && (
                        <div className="text-xs text-[var(--text-secondary)] font-mono">
                          <span>Engine: {getEngineName(answerMetadata.engine)}</span>
                          {answerMetadata.time !== null && answerMetadata.time !== undefined && (
                            <span className="ml-2">• {answerMetadata.time} ms</span>
                          )}
                        </div>
                      )}
                    </div>

                    <div className="answer-content">
                      <p className="text-sm leading-relaxed">
                        {typeof answer === 'string' ? answer : 'Invalid answer format'}
                      </p>
                    </div>

                    {answerMetadata?.sources && Array.isArray(answerMetadata.sources) && answerMetadata.sources.length > 0 && (
                      <div className="answer-sources">
                        <div className="text-xs text-[var(--text-secondary)] font-mono mb-1.5">Sources:</div>
                        <div className="flex flex-wrap gap-1.5">
                          {answerMetadata.sources.map((source, idx) => (
                            <span
                              key={idx}
                              className="text-xs font-mono px-2 py-0.5 border border-[var(--border)] text-[var(--text-secondary)]"
                            >
                              Chunk {typeof source === 'string' ? source : idx + 1}
                            </span>
                          ))}
                        </div>
                      </div>
                    )}
                  </div>
                )}
              </AnswerErrorBoundary>
            </div>
          </div>

          {/* ROW 2: Search Engine Status */}
          <section className="engine-section">
            <div className="section-heading">
              <h2 className="font-mono">Search Engine Status</h2>
              <p className="font-mono">Vector search engines powering VectorDB</p>
            </div>

            <div className="engine-grid">
              {Object.entries(engineResults).map(([key, result]) => {
                const engineColor = getEngineColor(key);
                const statusColor = getStatusColor(result.status);
                const engineKey = key === 'hnsw' ? 'hnsw' : key === 'kdTree' ? 'kd-tree' : 'brute-force';
                const cardClass = `engine-card ${engineKey} ${result.fastest ? 'fastest' : ''}`;

                return (
                  <div key={key} className={cardClass}>
                    <div className="engine-card-header">
                      <div className="w-7 h-7 border border-[var(--border)] flex items-center justify-center">
                        {key === 'hnsw' && <Icon type="hnsw" className="w-4 h-4" style={{ color: engineColor }} />}
                        {key === 'kdTree' && <Icon type="kdtree" className="w-4 h-4" style={{ color: engineColor }} />}
                        {key === 'bruteForce' && <Icon type="brute" className="w-4 h-4" style={{ color: engineColor }} />}
                      </div>
                      <span className="engine-name font-mono" style={{ color: engineColor }}>{getEngineName(key)}</span>
                    </div>

                    <p className="engine-description font-mono">{getEngineDescription(key)}</p>

                    <div className="engine-divider"></div>

                    <div className="engine-meta font-mono">
                      <div className="flex items-center gap-2">
                        <span>Status:</span>
                        <span className={statusColor}>{result.status}</span>
                      </div>

                      {result.time !== null && result.time !== undefined && (
                        <div className="flex items-center gap-2">
                          <span>Time:</span>
                          <span className="text-[var(--text-primary)]">{result.time} ms</span>
                        </div>
                      )}

                      {result.chunks !== null && result.chunks !== undefined && (
                        <div className="flex items-center gap-2">
                          <span>Chunks:</span>
                          <span className="text-[var(--text-primary)]">{result.chunks}</span>
                        </div>
                      )}

                      {result.error && (
                        <div className="flex items-center gap-2">
                          <span>Error:</span>
                          <span className="text-[var(--error)]">{result.error}</span>
                        </div>
                      )}
                    </div>

                    {result.fastest && (
                      <div className="fastest-badge font-mono">★ Fastest</div>
                    )}
                  </div>
                );
              })}
            </div>
          </section>
        </main>

        {/* Footer */}
        <footer className="app-footer font-mono">
          <p>PDF → Text → Chunks → Embeddings → Vector Search → AI Answer</p>
          <p className="mt-1">Powered by C++ • Custom HNSW • Ollama • Semantic Search</p>
        </footer>
      </div>
    </div>
  );
}

export default App;