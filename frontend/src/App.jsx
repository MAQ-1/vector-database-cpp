import { useState } from 'react'
import SearchPanel from './components/SearchPanel'
import SearchResults from './components/SearchResults'
import BenchmarkResults from './components/BenchmarkResults'

function App() {
  const [searchResults, setSearchResults] = useState([])
  const [benchmarkData, setBenchmarkData] = useState(null)
  const [isLoading, setIsLoading] = useState(false)
  const [isBenchmarkLoading, setIsBenchmarkLoading] = useState(false)
  const [activeView, setActiveView] = useState('search')

  const handleSearch = (searchParams) => {
    console.log('Search initiated with:', searchParams)
    setIsLoading(true)
    
    // Simulate search delay
    setTimeout(() => {
      // Mock search results for UI demonstration
      const mockResults = [
        { id: 1, rank: 1, score: 0.95, metadata: 'Document about machine learning' },
        { id: 2, rank: 2, score: 0.87, metadata: 'AI research paper' },
        { id: 3, rank: 3, score: 0.82, metadata: 'Neural network tutorial' },
        { id: 4, rank: 4, score: 0.76, metadata: 'Deep learning guide' },
        { id: 5, rank: 5, score: 0.68, metadata: 'Vector database documentation' }
      ]
      setSearchResults(mockResults)
      setIsLoading(false)
    }, 500)
  }

  const handleBenchmark = () => {
    console.log('Benchmark initiated')
    setIsBenchmarkLoading(true)
    
    // Simulate benchmark delay
    setTimeout(() => {
      // Mock benchmark data for UI demonstration
      const mockBenchmarkData = {
        'Brute Force': 4010,
        'KD-Tree': 0.35,
        'LSH': 0.10,
        'HNSW': 20.59
      }
      setBenchmarkData(mockBenchmarkData)
      setIsBenchmarkLoading(false)
    }, 800)
  }

  const navItems = [
    {
      group: 'WORKSPACE',
      items: [
        {
          id: 'search',
          label: 'Search',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <circle cx="11" cy="11" r="8"/>
              <path d="M21 21l-4.35-4.35"/>
            </svg>
          )
        },
        {
          id: 'documents',
          label: 'Documents',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
              <polyline points="14 2 14 8 20 8"/>
              <line x1="16" y1="13" x2="8" y2="13"/>
              <line x1="16" y1="17" x2="8" y2="17"/>
            </svg>
          )
        },
        {
          id: 'ai',
          label: 'Ask AI',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>
            </svg>
          )
        }
      ]
    },
    {
      group: 'ANALYSIS',
      items: [
        {
          id: 'benchmark',
          label: 'Benchmark',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <line x1="18" y1="20" x2="18" y2="10"/>
              <line x1="12" y1="20" x2="12" y2="4"/>
              <line x1="6" y1="20" x2="6" y2="14"/>
            </svg>
          )
        },
        {
          id: 'hnsw',
          label: 'HNSW',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <circle cx="12" cy="5" r="2"/>
              <circle cx="5" cy="12" r="2"/>
              <circle cx="19" cy="12" r="2"/>
              <line x1="12" y1="5" x2="5" y2="12"/>
              <line x1="12" y1="5" x2="19" y2="12"/>
            </svg>
          )
        }
      ]
    },
    {
      group: 'SYSTEM',
      items: [
        {
          id: 'status',
          label: 'Status',
          icon: (
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
              <circle cx="12" cy="12" r="10"/>
              <circle cx="12" cy="12" r="3"/>
            </svg>
          )
        }
      ]
    }
  ]

  const placeholderViews = {
    documents: {
      title: 'Documents',
      description: 'Manage documents and generate vector embeddings.',
      copy: 'Document workspace coming next.',
      icon: (
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
          <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
          <polyline points="14 2 14 8 20 8"/>
          <line x1="16" y1="13" x2="8" y2="13"/>
          <line x1="16" y1="17" x2="8" y2="17"/>
        </svg>
      )
    },
    ai: {
      title: 'Ask AI',
      description: 'Ask questions using retrieved document context.',
      copy: 'RAG workspace coming next.',
      icon: (
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
          <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>
        </svg>
      )
    },
    benchmark: {
      title: 'Benchmark',
      description: 'Compare vector search algorithm performance.',
      copy: 'Benchmark workspace coming next.',
      icon: (
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
          <line x1="18" y1="20" x2="18" y2="10"/>
          <line x1="12" y1="20" x2="12" y2="4"/>
          <line x1="6" y1="20" x2="6" y2="14"/>
        </svg>
      )
    },
    hnsw: {
      title: 'HNSW',
      description: 'Explore the HNSW index and graph structure.',
      copy: 'HNSW workspace coming next.',
      icon: (
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
          <circle cx="12" cy="5" r="2"/>
          <circle cx="5" cy="12" r="2"/>
          <circle cx="19" cy="12" r="2"/>
          <line x1="12" y1="5" x2="5" y2="12"/>
          <line x1="12" y1="5" x2="19" y2="12"/>
        </svg>
      )
    },
    status: {
      title: 'System Status',
      description: 'Monitor VectorDB and Ollama.',
      copy: 'System status workspace coming next.',
      icon: (
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-4">
          <circle cx="12" cy="12" r="10"/>
          <circle cx="12" cy="12" r="3"/>
        </svg>
      )
    }
  }

  const renderPlaceholderView = (viewKey) => {
    const view = placeholderViews[viewKey]
    return (
      <>
        <div>
          <h1 className="text-2xl font-semibold tracking-tight text-slate-100">{view.title}</h1>
          <p className="mt-1 text-sm text-slate-400">{view.description}</p>
        </div>
        
        <div className="rounded-lg border border-white/[0.06] bg-[#101722] px-6 py-10 flex flex-col items-center text-center gap-2 max-w-xl">
          <div className="size-8 rounded-md bg-white/[0.04] flex items-center justify-center text-slate-500">
            {view.icon}
          </div>
          <p className="text-sm text-slate-500">{view.copy}</p>
        </div>
      </>
    )
  }

  const renderMainContent = () => {
    switch (activeView) {
      case 'search':
        return (
          <>
            <div>
              <h1 className="text-2xl font-semibold tracking-tight text-slate-100">Vector Search</h1>
              <p className="mt-1 text-sm text-slate-400">Find nearest vectors using your selected search algorithm.</p>
            </div>
            
            <SearchPanel 
              onSearch={handleSearch} 
              onBenchmark={handleBenchmark}
              isLoading={isLoading}
              isBenchmarkLoading={isBenchmarkLoading}
            />

            <SearchResults 
              results={searchResults} 
              isLoading={isLoading}
              searchType="similarity"
            />

            <BenchmarkResults 
              data={benchmarkData} 
              isLoading={isBenchmarkLoading}
            />
          </>
        )
      case 'documents':
        return renderPlaceholderView('documents')
      case 'ai':
        return renderPlaceholderView('ai')
      case 'benchmark':
        return renderPlaceholderView('benchmark')
      case 'hnsw':
        return renderPlaceholderView('hnsw')
      case 'status':
        return renderPlaceholderView('status')
      default:
        return null
    }
  }

  return (
    <div className="flex h-screen w-screen overflow-hidden bg-[#080B12] text-slate-100">
      {/* Header */}
      <header className="h-14 shrink-0 flex items-center justify-between px-4 bg-[#090D14] border-b border-white/[0.06] fixed top-0 left-0 right-0 z-10">
        <div className="flex items-center gap-2.5">
          <div className="size-7 rounded-md bg-violet-600 flex items-center justify-center">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" className="size-4 text-slate-100">
              <path d="M12 2L2 7l10 5 10-5-10-5z"/>
              <path d="M2 17l10 5 10-5"/>
              <path d="M2 12l10 5 10-5"/>
            </svg>
          </div>
          <span className="text-[15px] font-semibold tracking-tight text-slate-100">VectorDB</span>
          <span className="hidden md:inline text-xs text-slate-500">Vector Search Engine</span>
        </div>
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-1.5">
            <span className="size-1.5 rounded-full bg-green-500" />
            <span className="text-xs text-slate-400">Backend</span>
          </div>
          <div className="flex items-center gap-1.5">
            <span className="size-1.5 rounded-full bg-green-500" />
            <span className="text-xs text-slate-400">Ollama</span>
          </div>
        </div>
      </header>

      {/* Main layout */}
      <div className="flex flex-1 pt-14 overflow-hidden">
        {/* Sidebar */}
        <aside className="w-64 max-md:w-56 shrink-0 flex flex-col justify-between bg-[#0B1018] border-r border-white/[0.06] overflow-y-auto">
          <nav className="flex flex-col gap-6 px-3 py-5">
            {navItems.map((group, groupIndex) => (
              <div key={groupIndex} className="flex flex-col gap-1">
                <h3 className="px-3 pb-1 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                  {group.group}
                </h3>
                {group.items.map((item) => (
                  <button
                    key={item.id}
                    onClick={() => setActiveView(item.id)}
                    className={`flex items-center gap-2.5 px-3 py-2 rounded-md text-sm font-medium transition-colors duration-150 ${
                      activeView === item.id
                        ? 'bg-violet-500/10 text-violet-400 border border-violet-500/20'
                        : 'text-slate-400 hover:bg-white/[0.04] hover:text-slate-200'
                    }`}
                  >
                    <span className={activeView === item.id ? 'text-violet-400' : 'text-slate-500'}>
                      {item.icon}
                    </span>
                    {item.label}
                  </button>
                ))}
              </div>
            ))}
          </nav>
          
          
        </aside>

        {/* Main Content */}
        <main className="flex-1 overflow-y-auto bg-[#080B12] px-6 py-8 lg:px-8">
          <div className="max-w-[1400px] w-full flex flex-col gap-6">
            {renderMainContent()}
          </div>
        </main>
      </div>
    </div>
  )
}

export default App