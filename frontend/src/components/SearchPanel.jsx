import { useState } from 'react'

const SearchPanel = ({ onSearch, onBenchmark, isLoading, isBenchmarkLoading }) => {
  const [queryVector, setQueryVector] = useState('0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0')
  const [algorithm, setAlgorithm] = useState('HNSW')
  const [metric, setMetric] = useState('Cosine')
  const [kValue, setKValue] = useState(5)

  const algorithms = ['HNSW', 'KD-Tree', 'LSH', 'Brute Force']
  const metrics = ['Cosine', 'Euclidean', 'Manhattan']

  const handleSearch = () => {
    if (!queryVector.trim()) {
      alert('Please enter a query vector')
      return
    }

    // Validate query vector format
    const vectorArray = queryVector.split(',').map(num => parseFloat(num.trim()))
    const isValidVector = vectorArray.every(num => !isNaN(num))
    
    if (!isValidVector) {
      alert('Invalid query vector format. Please enter comma-separated numbers.')
      return
    }

    const searchParams = {
      queryVector: vectorArray,
      algorithm,
      metric,
      k: kValue
    }

    onSearch(searchParams)
  }

  const handleBenchmark = () => {
    if (!queryVector.trim()) {
      alert('Please enter a query vector first')
      return
    }

    const vectorArray = queryVector.split(',').map(num => parseFloat(num.trim()))
    const isValidVector = vectorArray.every(num => !isNaN(num))
    
    if (!isValidVector) {
      alert('Invalid query vector format. Please enter comma-separated numbers.')
      return
    }

    onBenchmark()
  }

  const handleExampleVector = () => {
    setQueryVector('0.9,0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.0')
  }

  const handleRandomVector = () => {
    const randomVector = Array.from({ length: 10 }, () => 
      (Math.random() * 2 - 1).toFixed(2)
    ).join(',')
    setQueryVector(randomVector)
  }

  const dimensionCount = queryVector.split(',').filter(x => x.trim()).length

  return (
    <div className="bg-[#101722] border border-white/[0.06] rounded-xl p-6 shadow-sm">
      {/* Section Header */}
      <div className="mb-6">
        <h2 className="text-lg font-semibold tracking-tight text-slate-100">Search Configuration</h2>
        <p className="mt-1 text-sm text-slate-400">Configure the query and retrieval strategy.</p>
      </div>

      {/* Query Vector - Primary Control */}
      <div className="mb-6">
        <div className="flex items-center justify-between mb-2.5">
          <label htmlFor="query-vector" className="text-xs font-medium uppercase tracking-wider text-slate-500">
            Query Vector
          </label>
          <span className="text-xs font-mono tabular-nums text-cyan-400/80">
            {dimensionCount} dimensions
          </span>
        </div>
        
        <textarea
          id="query-vector"
          className="w-full bg-[#0B111A] border border-white/[0.08] rounded-lg px-4 py-3.5 text-sm font-mono text-slate-200 placeholder-slate-600
                     focus:outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/20
                     transition-colors duration-200 resize-none"
          value={queryVector}
          onChange={(e) => setQueryVector(e.target.value)}
          placeholder="0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.0"
          rows={3}
        />

        <div className="flex items-center gap-2 mt-2.5">
          <button 
            type="button"
            onClick={handleExampleVector}
            className="text-xs px-2.5 py-1.5 rounded-md bg-white/[0.03] border border-white/[0.08] text-slate-500 
                       hover:text-slate-300 hover:border-white/[0.15] hover:bg-white/[0.05]
                       transition-colors duration-150 font-medium"
          >
            Example
          </button>
          <button 
            type="button"
            onClick={handleRandomVector}
            className="text-xs px-2.5 py-1.5 rounded-md bg-white/[0.03] border border-white/[0.08] text-slate-500 
                       hover:text-slate-300 hover:border-white/[0.15] hover:bg-white/[0.05]
                       transition-colors duration-150 font-medium"
          >
            Random
          </button>
        </div>
      </div>

      {/* Configuration Controls Row */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
        <div>
          <label htmlFor="algorithm" className="block text-xs font-medium uppercase tracking-wider text-slate-500 mb-2">
            Search Algorithm
          </label>
          <select
            id="algorithm"
            className="w-full bg-[#0B111A] border border-white/[0.08] rounded-md px-3 py-2.5 text-sm font-mono text-slate-200 
                       focus:outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/20
                       transition-colors duration-200 appearance-none cursor-pointer
                       bg-[url('data:image/svg+xml;charset=utf-8,%3Csvg%20xmlns%3D%22http%3A//www.w3.org/2000/svg%22%20width%3D%2212%22%20height%3D%2212%22%20viewBox%3D%220%200%2012%2012%22%3E%3Cpath%20fill%3D%22%2364748B%22%20d%3D%22M3%204.5l3%203%203-3%22/%3E%3C/svg%3E')] bg-[length:12px] bg-[right_12px_center] bg-no-repeat pr-10"
            value={algorithm}
            onChange={(e) => setAlgorithm(e.target.value)}
          >
            {algorithms.map((algo) => (
              <option key={algo} value={algo} className="bg-[#101722] text-slate-200">{algo}</option>
            ))}
          </select>
        </div>

        <div>
          <label htmlFor="metric" className="block text-xs font-medium uppercase tracking-wider text-slate-500 mb-2">
            Similarity Metric
          </label>
          <select
            id="metric"
            className="w-full bg-[#0B111A] border border-white/[0.08] rounded-md px-3 py-2.5 text-sm font-mono text-slate-200 
                       focus:outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/20
                       transition-colors duration-200 appearance-none cursor-pointer
                       bg-[url('data:image/svg+xml;charset=utf-8,%3Csvg%20xmlns%3D%22http%3A//www.w3.org/2000/svg%22%20width%3D%2212%22%20height%3D%2212%22%20viewBox%3D%220%200%2012%2012%22%3E%3Cpath%20fill%3D%22%2364748B%22%20d%3D%22M3%204.5l3%203%203-3%22/%3E%3C/svg%3E')] bg-[length:12px] bg-[right_12px_center] bg-no-repeat pr-10"
            value={metric}
            onChange={(e) => setMetric(e.target.value)}
          >
            {metrics.map((m) => (
              <option key={m} value={m} className="bg-[#101722] text-slate-200">{m}</option>
            ))}
          </select>
        </div>

        <div>
          <label htmlFor="k-value" className="block text-xs font-medium uppercase tracking-wider text-slate-500 mb-2">
            Top-K Results
          </label>
          <input
            id="k-value"
            type="number"
            className="w-full bg-[#0B111A] border border-white/[0.08] rounded-md px-3 py-2.5 text-sm font-mono text-slate-200 
                       focus:outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/20
                       transition-colors duration-200"
            min="1"
            max="100"
            value={kValue}
            onChange={(e) => setKValue(Math.max(1, parseInt(e.target.value) || 1))}
          />
        </div>
      </div>

      {/* Divider */}
      <div className="border-t border-white/[0.06] mb-6" />

      {/* Action Buttons */}
      <div className="flex items-center gap-3 mb-6">
        <button
          type="button"
          onClick={handleSearch}
          disabled={isLoading}
          className="inline-flex items-center gap-2 px-4 py-2.5 text-sm font-semibold rounded-md
                     bg-violet-600 text-slate-100
                     hover:bg-violet-600/90 
                     focus:outline-none focus:ring-2 focus:ring-violet-500/40
                     disabled:opacity-50 disabled:cursor-not-allowed
                     transition-colors duration-150"
        >
          {isLoading ? (
            <>
              <span className="h-3.5 w-3.5 border-2 border-white/30 border-t-white/80 rounded-full animate-spin"></span>
              Searching...
            </>
          ) : (
            'SEARCH'
          )}
        </button>

        <button
          type="button"
          onClick={handleBenchmark}
          disabled={isBenchmarkLoading}
          className="inline-flex items-center gap-2 px-4 py-2.5 text-sm font-medium rounded-md
                     bg-white/[0.03] border border-white/[0.08] text-slate-400
                     hover:text-slate-200 hover:border-white/[0.15] hover:bg-white/[0.06]
                     focus:outline-none focus:ring-2 focus:ring-white/10
                     disabled:opacity-40 disabled:cursor-not-allowed
                     transition-colors duration-150"
        >
          {isBenchmarkLoading ? (
            <>
              <span className="h-3.5 w-3.5 border-2 border-white/20 border-t-white/50 rounded-full animate-spin"></span>
              Benchmarking...
            </>
          ) : (
            'COMPARE ALL ALGORITHMS'
          )}
        </button>
      </div>

      {/* Configuration Metadata */}
      <div className="flex items-center gap-6 text-xs">
        <div className="flex items-center gap-2">
          <span className="text-slate-500 font-medium">Current:</span>
          <span className="font-mono text-slate-400">
            {algorithm} <span className="text-slate-600 mx-1">·</span> {metric} <span className="text-slate-600 mx-1">·</span> Top-{kValue}
          </span>
        </div>
        <div className="flex items-center gap-2">
          <span className="text-slate-500 font-medium">Dimensions:</span>
          <span className="font-mono tabular-nums text-cyan-400/80">
            {dimensionCount}
          </span>
        </div>
      </div>
    </div>
  )
}

export default SearchPanel