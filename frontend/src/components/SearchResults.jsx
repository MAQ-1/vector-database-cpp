const SearchResults = ({ results, isLoading, searchType }) => {
  if (isLoading) {
    return (
      <div className="bg-[#101722] border border-white/[0.06] rounded-xl p-6 shadow-sm">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Search Results</h3>
          <span className="text-xs font-mono text-slate-500">Loading...</span>
        </div>
        <div className="flex flex-col items-center justify-center py-16 gap-4">
          <div className="h-8 w-8 border-2 border-violet-500/30 border-t-violet-400 rounded-full animate-spin"></div>
          <p className="text-sm text-slate-400">Searching vector database...</p>
        </div>
      </div>
    )
  }

  if (!results || results.length === 0) {
    return (
      <div className="bg-[#101722] border border-white/[0.06] rounded-xl p-6 shadow-sm">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Search Results</h3>
          <span className="text-xs font-mono text-slate-500">No results</span>
        </div>
        <div className="flex flex-col items-center justify-center py-16 gap-3">
          <div className="size-10 rounded-lg bg-white/[0.03] border border-white/[0.06] flex items-center justify-center text-slate-600">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-5">
              <circle cx="11" cy="11" r="8"/>
              <path d="M21 21l-4.35-4.35"/>
            </svg>
          </div>
          <h4 className="text-sm font-medium text-slate-300">No search performed yet</h4>
          <p className="text-sm text-slate-500 max-w-md text-center">Configure your search parameters and click SEARCH to find similar vectors</p>
        </div>
      </div>
    )
  }

  const avgScore = (results.reduce((sum, r) => sum + r.score, 0) / results.length).toFixed(4)

  return (
    <div className="bg-[#101722] border border-white/[0.06] rounded-xl shadow-sm overflow-hidden">
      {/* Results Header */}
      <div className="flex items-center justify-between px-6 py-4 border-b border-white/[0.06]">
        <div className="flex items-center gap-3">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Search Results</h3>
        </div>
        <div className="flex items-center gap-3">
          <span className="text-xs font-mono text-slate-500">
            {searchType === 'similarity' ? 'Similarity Score' : 'Distance'}
          </span>
          <span className="text-slate-600">·</span>
          <span className="text-xs font-mono text-slate-500">
            Top-{results.length} results
          </span>
        </div>
      </div>

      {/* Results Table */}
      <div className="overflow-x-auto">
        <table className="w-full">
          <thead>
            <tr className="border-b border-white/[0.06]">
              <th className="text-left px-6 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500 w-20">
                Rank
              </th>
              <th className="text-left px-4 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500 w-20">
                ID
              </th>
              <th className="text-left px-4 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                {searchType === 'similarity' ? 'Similarity Score' : 'Distance'}
              </th>
              <th className="text-left px-6 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                Metadata
              </th>
            </tr>
          </thead>
          <tbody>
            {results.map((result) => (
              <tr 
                key={result.id} 
                className="border-b border-white/[0.04] hover:bg-white/[0.02] transition-colors duration-150"
              >
                <td className="px-6 py-4">
                  <span className="text-xs font-mono text-slate-500 tabular-nums">
                    {String(result.rank).padStart(2, '0')}
                  </span>
                </td>
                <td className="px-4 py-4">
                  <code className="text-sm font-mono text-cyan-400/80">
                    {result.id}
                  </code>
                </td>
                <td className="px-4 py-4">
                  <div className="flex items-center gap-3">
                    <div className="flex-1 max-w-[120px]">
                      <div className="h-1.5 bg-white/[0.04] rounded-full overflow-hidden">
                        <div 
                          className="h-full bg-violet-500/40 rounded-full transition-all duration-300"
                          style={{ width: `${(result.score * 100).toFixed(0)}%` }}
                        />
                      </div>
                    </div>
                    <span className="text-sm font-mono font-medium text-slate-200 tabular-nums min-w-[4.5rem] text-right">
                      {result.score.toFixed(4)}
                    </span>
                    {searchType === 'similarity' && result.score > 0.8 && (
                      <span className="inline-flex items-center gap-1.5 text-[11px] text-emerald-400/80">
                        <span className="size-1 rounded-full bg-emerald-400/60"></span>
                        High match
                      </span>
                    )}
                  </div>
                </td>
                <td className="px-6 py-4">
                  <div className="flex flex-col gap-1.5">
                    <span className="text-sm text-slate-300">{result.metadata}</span>
                    <div className="flex items-center gap-2">
                      <span className="text-[11px] font-mono text-slate-600 bg-white/[0.03] border border-white/[0.06] px-1.5 py-0.5 rounded">
                        vector
                      </span>
                      <span className="text-[11px] font-mono text-slate-600 bg-white/[0.03] border border-white/[0.06] px-1.5 py-0.5 rounded">
                        embedding
                      </span>
                    </div>
                  </div>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {/* Results Footer */}
      <div className="flex items-center justify-between px-6 py-4 border-t border-white/[0.06] bg-white/[0.01]">
        <div className="flex items-center gap-6">
          <div className="flex items-center gap-2">
            <span className="text-xs text-slate-500">Best Match:</span>
            <span className="text-xs font-mono text-slate-300">
              ID {results[0].id}
            </span>
            <span className="text-xs font-mono font-medium text-violet-400">
              ({results[0].score.toFixed(4)})
            </span>
          </div>
          <div className="flex items-center gap-2">
            <span className="text-xs text-slate-500">Avg Score:</span>
            <span className="text-xs font-mono text-slate-400 tabular-nums">
              {avgScore}
            </span>
          </div>
        </div>
        <div className="flex items-center gap-2">
          <button 
            className="text-xs px-3 py-1.5 rounded-md bg-white/[0.02] border border-white/[0.06] text-slate-600 
                       cursor-not-allowed font-medium"
            disabled
          >
            Export CSV
          </button>
          <button 
            className="text-xs px-3 py-1.5 rounded-md bg-white/[0.02] border border-white/[0.06] text-slate-600 
                       cursor-not-allowed font-medium"
            disabled
          >
            View Details
          </button>
        </div>
      </div>
    </div>
  )
}

export default SearchResults